#include "externalGravityForce.h"
#include "../time_steppers/baseTimeStepper.h"

externalGravityForce::externalGravityForce(const vector<shared_ptr<elasticRod>>& m_limbs,
                                           const vector<shared_ptr<elasticJoint>>& m_joints,
                                           Vector3d m_gVector) :
                                           baseForce(m_limbs, m_joints), gVector(m_gVector)
{
    setGravity();
}

externalGravityForce::~externalGravityForce()
{
    ;
}

void externalGravityForce::computeFg()
{
    int limb_idx = 0;
    for (const auto& limb : limbs) {
        massGravity = massGravities[limb_idx];
        for (int i = 0; i < limb->ndof; i++)
        {
            if (limb->isDOFJoint[i]) continue;
            stepper->addForce(i, -massGravity[i], limb_idx); // subtracting gravity force
        }
        limb_idx++;
    }

    // TODO: store these values like above
    double force;
    for (const auto & joint : joints) {
        for (int i = 0; i < 3; i++) {
            force = gVector[i] * joint->mass;
            stepper->addForce(4*joint->joint_node+i, -force, joint->joint_limb);
        }
    }
}

void externalGravityForce::computeJg()
{
    ;
}

void externalGravityForce::setGravity()
{
    for (const auto& limb : limbs) {
        massGravity = VectorXd::Zero(limb->ndof);
        for (int i = 0; i < limb->nv; i++)
        {
            for (int k = 0; k < 3; k++)
            {
                int ind = 4*i + k;
                massGravity[ind] = gVector[k] * limb->massArray[ind];
            }
        }
        massGravities.push_back(massGravity);
    }
}
