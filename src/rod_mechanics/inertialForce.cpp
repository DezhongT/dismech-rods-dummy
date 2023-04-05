#include "inertialForce.h"

inertialForce::inertialForce(const vector<shared_ptr<elasticRod>>& m_limbs,
                             const vector<shared_ptr<elasticJoint>>& m_joints,
                             shared_ptr<baseTimeStepper> m_stepper)
{
    limbs = m_limbs;
    joints = m_joints;
    stepper = m_stepper;

    dt = limbs[0]->dt;
}

inertialForce::~inertialForce()
{
    ;
}

void inertialForce::computeFi()
{
    // TODOder: we should not need to compute this at every iteration.
    // We should compute and store it in iteration 1 and then reuse it.
    int limb_idx = 0;
    for (const auto& limb : limbs) {
        for (int i=0; i < limb->ndof; i++)
        {
            if (limb->isDOFJoint[i]) continue;
            f = (limb->massArray[i] / dt) * ((limb->x[i] - limb->x0[i]) / dt - limb->u[i]);
            stepper->addForce(i, f, limb_idx);
        }
        limb_idx++;
    }

    for (const auto& joint : joints) {
        for (int i = 0; i < 3; i++) {
            f = (joint->mass / dt) * ((joint->x[i] - joint->x0[i]) / dt - joint->u[i]);
            stepper->addForce(4*joint->joint_node+i, f,  joint->joint_limb);
        }
    }
}

void inertialForce::computeJi()
{
    int limb_idx = 0;
    for (const auto& limb : limbs) {
        for (int i = 0; i < limb->ndof; i++) {
            if (limb->isDOFJoint[i]) continue;
            jac = limb->massArray(i) / (dt * dt);
            stepper->addJacobian(i, i, jac, limb_idx);
        }
        limb_idx++;
    }

    int ind;
    for (const auto& joint : joints) {
        for (int i = 0; i < 3; i++) {
            jac = joint->mass / (dt * dt);
            ind = 4*joint->joint_node;
            stepper->addJacobian(ind+i, ind+i, jac, joint->joint_limb);
        }
    }
}
