#ifndef BASETIMESTEPPER_H
#define BASETIMESTEPPER_H

#include "../controllers/rodController.h"
#include "../rod_mechanics/elasticRod.h"
#include "../eigenIncludes.h"

// include force classes
#include "../rod_mechanics/elasticStretchingForce.h"
#include "../rod_mechanics/elasticBendingForce.h"
#include "../rod_mechanics/elasticTwistingForce.h"
#include "../rod_mechanics/inertialForce.h"

// include external force
#include "../rod_mechanics/dampingForce.h"
#include "../rod_mechanics/externalGravityForce.h"
#include "../rod_mechanics/floorContactForce.h"
#include "../rod_mechanics/contactPotentialIMC.h"


class baseTimeStepper : public enable_shared_from_this<baseTimeStepper>
{
public:
    baseTimeStepper(const vector<shared_ptr<elasticRod>>& m_limbs,
                    const vector<shared_ptr<elasticJoint>>& m_joints,
                    const vector<shared_ptr<rodController>>& m_controllers,
                    shared_ptr<elasticStretchingForce> m_stretchForce,
                    shared_ptr<elasticBendingForce> m_bendingForce,
                    shared_ptr<elasticTwistingForce> m_twistingForce,
                    shared_ptr<inertialForce> m_inertialForce,
                    shared_ptr<externalGravityForce> m_gravityForce,
                    shared_ptr<dampingForce> m_dampingForce,
                    shared_ptr<floorContactForce> m_floorContactForce,
                    double m_dt);
    virtual ~baseTimeStepper();

    void addForce(int ind, double p, int limb_idx);

    void setupForceStepperAccess();
    virtual void prepSystemForIteration();
    virtual void setZero();
    virtual void update();
    virtual void initSolver() = 0;
    virtual void integrator() = 0;
    virtual void addJacobian(int ind1, int ind2, double p, int limb_idx) = 0;
    virtual void addJacobian(int ind1, int ind2, double p, int limb_idx1, int limb_idx2) = 0;
    virtual void updateSystemForNextTimeStep() = 0;
    virtual void stepForwardInTime() = 0;

    double* dx;
    double* force;
    Map<VectorXd> Force;
    Map<VectorXd> DX;
    MatrixXd Jacobian;

    int freeDOF;
    vector<int> offsets;
    int iter = 0;


protected:
    int mappedInd, mappedInd1, mappedInd2;
    int offset;
    double dt;
    double alpha = 1.0;

    vector<shared_ptr<elasticRod>> limbs;
    vector<shared_ptr<elasticJoint>> joints;
    vector<shared_ptr<rodController>> controllers;
    shared_ptr<elasticStretchingForce> stretching_force;
    shared_ptr<elasticBendingForce> bending_force;
    shared_ptr<elasticTwistingForce> twisting_force;
    shared_ptr<inertialForce> inertial_force;
    shared_ptr<externalGravityForce> gravity_force;
    shared_ptr<dampingForce> damping_force;
    shared_ptr<floorContactForce> floor_contact_force;
};

#endif
