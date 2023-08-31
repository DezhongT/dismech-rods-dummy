#include "world.h"

world::world()
{
    ;
}

world::world(setInput &m_inputData) {
    render = m_inputData.GetBoolOpt("render");                          // boolean
    gVector = m_inputData.GetVecOpt("gVector");                         // m/s^2
    maxIter = m_inputData.GetIntOpt("maxIter");                         // maximum number of iterations
    rodRadius = m_inputData.GetScalarOpt("rodRadius");                  // meter
    youngM = m_inputData.GetScalarOpt("youngM");                        // Pa
    Poisson = m_inputData.GetScalarOpt("Poisson");                      // dimensionless
    deltaTime = m_inputData.GetScalarOpt("deltaTime");                  // seconds
    tol = m_inputData.GetScalarOpt("tol");                              // small number like 10e-7
    stol = m_inputData.GetScalarOpt("stol");                            // small number, e.g. 0.1%
    density = m_inputData.GetScalarOpt("density");                      // kg/m^3
    viscosity = m_inputData.GetScalarOpt("viscosity");                  // viscosity in Pa-s
    data_resolution = m_inputData.GetScalarOpt("dataResolution");       // time resolution for recording data
    col_limit = m_inputData.GetScalarOpt("colLimit");                   // distance limit for candidate set
    delta = m_inputData.GetScalarOpt("delta");                          // distance tolerance for contact
    k_scaler = m_inputData.GetScalarOpt("kScaler");                     // constant scaler for contact stiffness
    mu = m_inputData.GetScalarOpt("mu");                                // friction coefficient
    nu = m_inputData.GetScalarOpt("nu");                                // slipping tolerance for friction
    line_search = m_inputData.GetBoolOpt("lineSearch");                 // flag for enabling line search
    floor_z = m_inputData.GetScalarOpt("floorZ");                       // z-coordinate of floor plane
    totalTime = m_inputData.GetScalarOpt("simTime");                    // simulation duration
    integration_scheme = m_inputData.GetStringOpt("integrationScheme"); // integration scheme for time stepping
    phi_ctrl_filepath = m_inputData.GetStringOpt("phiCtrlFilePath");    // controller setpoints (bending angle phi for limbs)
    enable_2d_sim = m_inputData.GetBoolOpt("enable2DSim");              // flag for restricting sim to 2D
    adaptive_time_stepping = m_inputData.GetIntOpt("adaptiveTimeStepping");

    shearM = youngM / (2.0 * (1.0 + Poisson));                                // shear modulus

    data_rate = ceil(data_resolution / deltaTime);                         // iter resolution for recording data
    alpha = 1.0;                                                              // newton step size
}

world::~world()
{
    ;
}

bool world::isRender()
{
    return render;
}

void world::OpenFile(ofstream &outfile, string file_type)
{
    int systemRet = system("mkdir datafiles"); // make the directory
    if (systemRet == -1)
    {
        cout << "Error in creating directory\n";
    }

    // Open an input file named after the current time
    ostringstream file_name;
    file_name.precision(6);
    file_name << "datafiles/" << file_type;
    file_name << "_" << knot_config;
    file_name << "_dt_" << deltaTime;
    file_name << "_totalTime_" << totalTime;
    file_name << "_mu_" << mu;
    file_name << ".txt";
    outfile.open(file_name.str().c_str());
    outfile.precision(10);
}

// TODO: remove this in favor of the logging classes later
void world::outputNodeCoordinates(ofstream &outfile) {
//    if (timeStep % data_rate != 0) return;
//    Vector3d curr_node;
//    double curr_theta;
//    for (int i = 0; i < rod->nv-1; i++) {
//        curr_node = rod->getVertex(i);
//        curr_theta = rod->getTheta(i);
//        outfile << curr_node(0) << " " << curr_node(1) << " " <<
//                curr_node(2) << " " << curr_theta << endl;
//    }
//    curr_node = rod->getVertex(rod->nv-1);
//    outfile << curr_node(0) << " " << curr_node(1) << " " <<
//            curr_node(2) << " " << 0.0 << endl;
}

void world::CloseFile(ofstream &outfile)
{
    outfile.close();
}

void world::setupWorld(int argc, char**argv) {
    // TODO: make characteristicForce a function of total cumulative rod length?
    double temp_value = 1.0;
    // Find out the tolerance, e.g. how small is enough?
    characteristicForce = M_PI * pow(rodRadius, 4) / 4.0 * youngM / pow(temp_value, 2);
    forceTol = tol * characteristicForce;

    // Declare a constant external force. This is set in the robot description file
    shared_ptr<uniformConstantForce> uniform_force = nullptr;
    get_robot_description(argc, argv, limbs, joints, uniform_force, density, rodRadius, youngM, shearM);
    setupController(controllers, limbs, phi_ctrl_filepath);

    // This has to be called after joints are all set.
    for (const auto &joint : joints)
        joint->setup();

    // Declare inner elastic forces. These should never be optional.
    shared_ptr<elasticStretchingForce> stretching_force = make_shared<elasticStretchingForce>(limbs, joints);
    shared_ptr<elasticBendingForce> bending_force = make_shared<elasticBendingForce>(limbs, joints);
    shared_ptr<elasticTwistingForce> twisting_force = make_shared<elasticTwistingForce>(limbs, joints);

    // Declare inertial force. Only optional for verlet position scheme.
    shared_ptr<inertialForce> inertial_force = nullptr;
    if (integration_scheme != "verlet_position") {  // good enough for now, perhaps come up with cleaner method
        inertial_force = make_shared<inertialForce>(limbs, joints);
    }

    // Declare gravity
    shared_ptr<externalGravityForce> gravity_force = nullptr;
    if ((abs(gVector(0)) + abs(gVector(1)) + abs(gVector(2))) != 0) {
        gravity_force = make_shared<externalGravityForce>(limbs, joints, gVector);
    }

    // Declare viscous damping
    shared_ptr<dampingForce> damping_force = nullptr;
    if (viscosity != 0) {
        damping_force = make_shared<dampingForce>(limbs, joints, viscosity);
    }

    // Declare floor contact
    shared_ptr<floorContactForce> floor_contact_force = nullptr;
    if (floor_z != -9999) {  // come up with a better method for this later
        floor_contact_force = make_shared<floorContactForce>(limbs, joints, delta, nu, mu, floor_z);
    }

    inner_forces = make_shared<innerForces>(inertial_force, stretching_force, bending_force, twisting_force);
    external_forces = make_shared<externalForces>(gravity_force, damping_force, floor_contact_force, uniform_force);


    // set up the time stepper
    if (integration_scheme == "verlet_position") {
        stepper = make_shared<verletPosition>(limbs, joints, controllers, inner_forces, external_forces, deltaTime);
    }
    else if (integration_scheme == "backward_euler") {
        stepper = make_shared<backwardEuler>(limbs, joints, controllers, inner_forces, external_forces,
                                             deltaTime, forceTol, stol, maxIter, line_search,
                                             adaptive_time_stepping, PARDISO_SOLVER);
        stepper->initSolver();
    }
    else if (integration_scheme == "implicit_midpoint") {
        stepper = make_shared<implicitMidpoint>(limbs, joints, controllers, inner_forces, external_forces,
                                                deltaTime, forceTol, stol, maxIter, line_search,
                                                adaptive_time_stepping, PARDISO_SOLVER);
        stepper->initSolver();
    }
    else {
        cout << "Invalid integration scheme option was provided!" << endl;
        exit(1);
    }

    stepper->setupForceStepperAccess();

    if (enable_2d_sim) {
        for (auto& limb : limbs) limb->enable2DSim();
    }

    // Update boundary conditions
    updateCons();

    // Allocate every thing to prepare for the first iteration
    stepper->updateSystemForNextTimeStep();

    currentTime = 0.0;
    timeStep = 0;
}

void world::setupController(vector<shared_ptr<rodController>> &controllers, vector<shared_ptr<elasticRod>> &limbs, string phi_ctrl_filepath)
{
    if (phi_ctrl_filepath.empty()) return;
    int num_limb;
    num_limb = limbs.size();
    controllers.push_back(make_shared<rodOpenLoopFileKappabarSetter>(num_limb, phi_ctrl_filepath, limbs));
}

void world::updateCons()
{
    for (const auto &limb : limbs)
        limb->updateMap();
    stepper->update();
}

int world::getTimeStep()
{
    return timeStep;
}


void world::updateTimeStep() {
    currentTime += stepper->stepForwardInTime();
    timeStep++;
}

void world::printSimData()
{
    if (external_forces->floor_contact_force) {
        printf("time: %.4f | iters: %i | con: %i | min_dist: %.6f | k: %.3e | fric: %.1f\n",
               currentTime, stepper->iter, 0,
               external_forces->floor_contact_force->min_dist,
               0.0,
                //           m_collisionDetector->num_collisions,
                //           m_collisionDetector->min_dist,
                //           m_contactPotentialIMC->contact_stiffness,
               mu);
    }
    else {
        printf("time: %.4f | iters: %i | con: %i | min_dist: %s | k: %.3e | fric: %.1f\n",
               currentTime, stepper->iter, 0,
               "N/A",
               0.0,
                //           m_collisionDetector->num_collisions,
                //           m_collisionDetector->min_dist,
                //           m_contactPotentialIMC->contact_stiffness,
               mu);
    }
}

int world::simulationRunning() {
    if (currentTime < totalTime)
        return 1;
    else
    {
        cout << "Completed simulation." << endl;
        return -1;
    }
}

double world::getScaledCoordinate(int i, int limb_idx)
{
    return limbs[limb_idx]->x[i] / 0.2;
    //    return limbs[limb_idx]->x[i] / (0.20 * RodLength);
    //    return rod->x[i] / (0.20 * RodLength);
}

VectorXd world::getM1(int i, int limb_idx)
{
    return limbs[limb_idx]->m1.row(i);
}

VectorXd world::getM2(int i, int limb_idx)
{
    return limbs[limb_idx]->m2.row(i);
}

double world::getCurrentTime()
{
    return currentTime;
}
