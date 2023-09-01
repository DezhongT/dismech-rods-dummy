#ifndef IMPLICITTIMESTEPPER_H
#define IMPLICITTIMESTEPPER_H

#include "baseTimeStepper.h"
#include "solvers/solverTypes.h"
#include "mkl_types.h"

class baseSolver;


class implicitTimeStepper : public baseTimeStepper
{
public:
    implicitTimeStepper(const shared_ptr<softRobots>& m_soft_robots,
                        const shared_ptr<innerForces>& m_inner_forces,
                        const shared_ptr<externalForces>& m_external_forces,
                        double m_dt, double m_force_tol, double m_stol,
                        int m_max_iter, int m_line_search,
                        int m_adaptive_time_stepping, solverType m_solver_type);
    ~implicitTimeStepper() override;

    void addJacobian(int ind1, int ind2, double p, int limb_idx) override;
    void addJacobian(int ind1, int ind2, double p, int limb_idx1, int limb_idx2) override;
    void setZero() override;
    void update() override;
    void integrator() override;
    void initSolver() override;

    void prepSystemForIteration() override;
    virtual double newtonMethod(double dt) = 0;
    virtual void lineSearch(double dt) = 0;

    // Utility variables for PARDISO solver.
    // Need to keep track of non-zero elements
    // in implicitTimeStepper to avoid n^2 construction later.
    // This allows us to keep complexity to nlogn.
    MKL_INT* ia;
    vector<pair<int, int>> non_zero_elements;

    // For dgbsv solver
    double *dgbsv_jacobian;
    int kl, ku, num_rows;

protected:
    double force_tol;
    double stol;
    int max_iter;
    int line_search;

    template<solverType solver_type>
    void addJacobian(int ind1, int ind2, double p, int limb_idx1, int limb_idx2);

    double orig_dt;
    bool adaptive_time_stepping;
    int adaptive_time_stepping_threshold;


private:
    shared_ptr<implicitTimeStepper> shared_from_this();
    unique_ptr<baseSolver> solver;
    solverType solver_type;
    int dgbsv_jacobian_len;
};


#endif
