#include "implicitMidpoint.h"

implicitMidpoint::implicitMidpoint(const vector<shared_ptr<elasticRod>>& m_limbs,
                                   const vector<shared_ptr<elasticJoint>>& m_joints,
                                   const vector<shared_ptr<rodController>>& m_controllers,
                                   shared_ptr<elasticStretchingForce> m_stretch_force,
                                   shared_ptr<elasticBendingForce> m_bending_force,
                                   shared_ptr<elasticTwistingForce> m_twisting_force,
                                   shared_ptr<inertialForce> m_inertial_force,
                                   shared_ptr<externalGravityForce> m_gravity_force,
                                   shared_ptr<dampingForce> m_damping_force,
                                   shared_ptr<floorContactForce> m_floor_contact_force,
                                   double m_dt, double m_force_tol, double m_stol,
                                   int m_max_iter, int m_line_search, solverType m_solver_type) :
                                   backwardEuler(m_limbs, m_joints, m_controllers, m_stretch_force, m_bending_force,
                                                 m_twisting_force, m_inertial_force, m_gravity_force,
                                                 m_damping_force, m_floor_contact_force, m_dt,
                                                 m_force_tol, m_stol, m_max_iter, m_line_search, m_solver_type)

{
}

implicitMidpoint::~implicitMidpoint() = default;


void implicitMidpoint::stepForwardInTime() {
    for (const auto& limb : limbs) limb->updateGuess(0.01, 0.5 * dt);
    newtonMethod(0.5 * dt);
    for (const auto& limb : limbs) {
        limb->x = 2 * limb->x - limb->x0;
    }
    updateSystemForNextTimeStep();
}