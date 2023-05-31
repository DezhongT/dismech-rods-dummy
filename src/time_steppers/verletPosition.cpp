#include "verletPosition.h"


verletPosition::verletPosition(const vector<shared_ptr<elasticRod>> &m_limbs,
                               const vector<shared_ptr<elasticJoint>> &m_joints,
                               const vector<shared_ptr<rodController>> &m_controllers,
                               shared_ptr<elasticStretchingForce> m_stretch_force,
                               shared_ptr<elasticBendingForce> m_bending_force,
                               shared_ptr<elasticTwistingForce> m_twisting_force,
                               shared_ptr<inertialForce> m_inertial_force,
                               shared_ptr<externalGravityForce> m_gravity_force,
                               shared_ptr<dampingForce> m_damping_force,
                               shared_ptr<floorContactForce> m_floor_contact_force, double m_dt) :
                               explicitTimeStepper(m_limbs, m_joints, m_controllers, m_stretch_force, m_bending_force,
                                                   m_twisting_force, m_inertial_force, m_gravity_force,
                                                   m_damping_force, m_floor_contact_force, m_dt)

{
    constructInverseMassVector();
}


verletPosition::~verletPosition() = default;


void verletPosition::constructInverseMassVector() {
    int total_dof = 0;
    for (const auto& limb : limbs) {
        VectorXd curr_inv_masses = VectorXd::Zero(limb->ndof);
        for (int i = 0; i < limb->ndof; i++) {
            curr_inv_masses[i] = 1 / limb->massArray[i];
        }
        inverse_masses.push_back(curr_inv_masses);
        total_dof += limb->ndof;
    }

    // Replace the masses for the proper ones stored in joints
    for (const auto& joint : joints) {
        int j_node = joint->joint_node;
        int j_limb = joint->joint_limb;
        double inv_mass = 1 / joint->mass;
        inverse_masses[j_limb][4*j_node] = inv_mass;
        inverse_masses[j_limb][4*j_node+1] = inv_mass;
        inverse_masses[j_limb][4*j_node+2] = inv_mass;
    }
}


void verletPosition::stepForwardInTime() {
    // First position half step
    // Update q_t+dt/2 = q_t + v_t*dt/2
    for (const auto& limb : limbs) {
        limb->x = limb->x0 + limb->u * 0.5 * dt;
    }

    // Evaluation of local accelerations
    // compute F(q_t+dt/2)
    // Make sure to leave out inertial force
    prepSystemForIteration();
    stretching_force->computeFs();
    bending_force->computeFb();
    twisting_force->computeFt();
    gravity_force->computeFg();
    damping_force->computeFd(0.5 * dt);
    floor_contact_force->computeFf(0.5 * dt);

    // Could perhaps explore a vectorized solution for this later but too complicated for now.
    int counter = 0;
    double acceleration;
    int limb_num = 0;
    for (const auto& limb : limbs) {
        for (int local_counter = 0; local_counter < limb->ndof; local_counter++) {
            if (!limb->isConstrained[local_counter] && limb->isDOFJoint[local_counter] != 1) {
                // Computing accelerations
                // a_t+dt/2 = M^{-1} @ F
                acceleration = inverse_masses[limb_num][local_counter] * -Force[counter];

                // Update velocity u_t+dt = u_t + a_t+dt/2 * dt
                limb->u[local_counter] = limb->u[local_counter] + acceleration * dt;

                // Update position x_t+dt = x_t+dt/2 + u_t+dt * dt / 2
                limb->x[local_counter] = limb->x[local_counter] + limb->u[local_counter] * dt / 2;

                counter++;
            }
        }
        limb_num++;
    }
    updateSystemForNextTimeStep();
}


void verletPosition::updateSystemForNextTimeStep() {
    for (const auto& controller : controllers) {
        controller->updateTimestep(dt);
    }
    // Update x0
    for (const auto& limb : limbs) {
        limb->x0 = limb->x;
    }
}

