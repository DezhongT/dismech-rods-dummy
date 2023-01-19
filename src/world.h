#ifndef WORLD_H
#define WORLD_H

#include "eigenIncludes.h"
#include "robotDescription.h"

// include elastic rod class
#include "rod_mechanics/elasticRod.h"
#include "rod_mechanics/elasticJoint.h"

// include force classes
#include "rod_mechanics/elasticStretchingForce.h"
#include "rod_mechanics/elasticBendingForce.h"
#include "rod_mechanics/elasticTwistingForce.h"
#include "rod_mechanics/inertialForce.h"

// include external force
#include "rod_mechanics/dampingForce.h"
#include "rod_mechanics/externalGravityForce.h"
#include "rod_mechanics/floorContactForce.h"
//#include "rod_mechanics/contactPotentialIMC.h"

// include time stepper
#include "rod_mechanics/timeStepper.h"

// include input file and option
#include "initialization/setInput.h"

class world
{
public:
    world();
    world(setInput &m_inputData);
    ~world();
    void setupWorld();
    void updateTimeStep();
    int simulationRunning();
    double getScaledCoordinate(int i, int limb_idx);
    double getCurrentTime();

    bool isRender();

    // file output
    void OpenFile(ofstream &outfile, string filename);
    void CloseFile(ofstream &outfile);
    void outputNodeCoordinates(ofstream &outfile);

    int getTimeStep();

    // TODO: Create more sophisticated classes for these
    vector<shared_ptr<elasticRod>> limbs;
    vector<shared_ptr<elasticJoint>> joints;

private:

    // Physical parameters
    double rodRadius;
    double youngM;
    double Poisson;
    double shearM;
    double deltaTime;
    double density;
    Vector3d gVector;
    double viscosity;
    double col_limit;
    double delta;
    double k_scaler;
    double mu;
    double nu;
    double data_resolution;
    int data_rate;
    int line_search;
    string knot_config;
    double alpha;
    double floor_z;

    double tol, stol;
    int maxIter; // maximum number of iterations
    double characteristicForce;
    double forceTol;

    // Geometry
    MatrixXd vertices;
    VectorXd theta;

    // Rod
    shared_ptr<elasticRod> rod = nullptr;

    // set up the time stepper
    shared_ptr<timeStepper> stepper = nullptr;
    double *totalForce;
    double *dx;

    double currentTime;
    int timeStep;
    double totalTime;

    // declare the forces
    unique_ptr<elasticStretchingForce> m_stretchForce = nullptr;
    unique_ptr<elasticBendingForce> m_bendingForce = nullptr;
    unique_ptr<elasticTwistingForce> m_twistingForce = nullptr;
    unique_ptr<inertialForce> m_inertialForce = nullptr;
    unique_ptr<externalGravityForce> m_gravityForce = nullptr;
    unique_ptr<dampingForce> m_dampingForce = nullptr;
    unique_ptr<floorContactForce> m_floorContactForce = nullptr;
//    shared_ptr<collisionDetector> m_collisionDetector = nullptr;
//    unique_ptr<contactPotentialIMC> m_contactPotentialIMC = nullptr;

    int iter;
    int total_iters;

    void lockEdge(int edge_num, int limb_idx);

    void updateRobot();
    void prepRobot();

    void updateCons();

    void newtonMethod(bool &solved);
    void printSimData();
    void lineSearch();

    bool render; // should the OpenGL rendering be included?

    Vector3d temp;
    Vector3d temp1;
    Vector3d gravity;
    Vector3d inertial;
    Vector3d dampingF;
};

#endif
