#include "implicitTimeStepper.h"
#include <algorithm>


implicitTimeStepper::implicitTimeStepper(const vector<shared_ptr<elasticRod>>& m_limbs,
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
                                         int m_max_iter, int m_line_search) :
                                         baseTimeStepper(m_limbs, m_joints, m_controllers, m_stretch_force, m_bending_force,
                                                         m_twisting_force, m_inertial_force, m_gravity_force,
                                                         m_damping_force, m_floor_contact_force, m_dt),
                                         force_tol(m_force_tol), stol(m_stol), max_iter(m_max_iter), line_search(m_line_search)
{
//    kl = 10; // lower diagonals
//    ku = 10; // upper diagonals
//    ldb = freeDOF;
//    NUMROWS = 2 * kl + ku + 1;
//    ipiv = new int[freeDOF];
//    jacobianLen = (2 * kl + ku + 1) * freeDOF;
    nrhs = 1;
    info = 0;
//    jacobian = new double [jacobianLen];
    Jacobian = MatrixXd::Zero(freeDOF, freeDOF);

    ia = new MKL_INT[freeDOF+1]{0};
    ia[0] = 1;

    mtype = 11;           /* Real unsymmetric matrix */

    maxfct = 1;           /* Maximum number of numerical factorizations. */
    mnum = 1;             /* Which factorization to use. */
    msglvl = 0;           /* Print statistical information  */
    error = 0;            /* Initialize error flag */

/* -------------------------------------------------------------------- */
/* .. Initialize the internal solver memory pointer. This is only */
/* necessary for the FIRST call of the PARDISO solver. */
/* -------------------------------------------------------------------- */
    for (int i = 0; i < 64; i++ )
    {
        pt[i] = 0;
    }

/* -------------------------------------------------------------------- */
/* .. Setup Pardiso control parameters. */
/* -------------------------------------------------------------------- */
    for (int i = 0; i < 64; i++ )
    {
        iparm[i] = 0;
    }
    iparm[0] = 0;         /* No solver default */
    iparm[1] = 2;         /* Fill-in reordering from METIS */
    iparm[3] = 0;         /* No iterative-direct algorithm */
    iparm[4] = 0;         /* No user fill-in reducing permutation */
    iparm[5] = 0;         /* Write solution into x */
    iparm[6] = 0;         /* Not in use */
    iparm[7] = 2;         /* Max numbers of iterative refinement steps */
    iparm[8] = 0;         /* Not in use */
    iparm[9] = 13;        /* Perturb the pivot elements with 1E-13 */
    iparm[10] = 1;        /* Use nonsymmetric permutation and scaling MPS */
    iparm[11] = 0;        /* Conjugate transposed/transpose solve */
    iparm[12] = 1;        /* Maximum weighted matching algorithm is switched-on (default for non-symmetric) */
    iparm[13] = 0;        /* Output: Number of perturbed pivots */
    iparm[14] = 0;        /* Not in use */
    iparm[15] = 0;        /* Not in use */
    iparm[16] = 0;        /* Not in use */
    iparm[17] = -1;       /* Output: Number of nonzeros in the factor LU */
    iparm[18] = -1;       /* Output: Mflops for LU factorization */
    iparm[19] = 0;        /* Output: Numbers of CG Iterations */
}


implicitTimeStepper::~implicitTimeStepper() {
    delete [] ipiv;
    delete [] jacobian;
    delete [] ia;
}


double* implicitTimeStepper::getJacobian()
{
    return jacobian;
}


void implicitTimeStepper::addJacobian(int ind1, int ind2, double p, int limb_idx)
{
    shared_ptr<elasticRod> limb = limbs[limb_idx];
    mappedInd1 = limb->fullToUnconsMap[ind1];
    mappedInd2 = limb->fullToUnconsMap[ind2];
    offset = offsets[limb_idx];
    int jac_ind1, jac_ind2;
    if (limb->getIfConstrained(ind1) == 0 && limb->getIfConstrained(ind2) == 0) // both are free
    {
        jac_ind1 = mappedInd2+offset;
        jac_ind2 = mappedInd1+offset;
        if (Jacobian(jac_ind1, jac_ind2) == 0 && p != 0) {
            ia[jac_ind1+1]++;
            non_zero_elements.emplace_back(jac_ind1, jac_ind2);
        }
        else if (Jacobian(jac_ind1, jac_ind2) != 0 && Jacobian(jac_ind1, jac_ind2) + p == 0) {
            ia[jac_ind1+1]--;
            // This is expensive, but it doesn't happen that often. Better than using a set
            non_zero_elements.erase(remove(non_zero_elements.begin(),
                                           non_zero_elements.end(),
                                           pair<int, int>(jac_ind1, jac_ind2)), non_zero_elements.end());
        }
        Jacobian(jac_ind1, jac_ind2) += p;
    }
}

void implicitTimeStepper::addJacobian(int ind1, int ind2, double p, int limb_idx1, int limb_idx2) {
    shared_ptr<elasticRod> limb1 = limbs[limb_idx1];
    shared_ptr<elasticRod> limb2 = limbs[limb_idx2];
    mappedInd1 = limb1->fullToUnconsMap[ind1];
    mappedInd2 = limb2->fullToUnconsMap[ind2];
    int offset1 = offsets[limb_idx1];
    int offset2 = offsets[limb_idx2];
    int jac_ind1, jac_ind2;
    if (limb1->getIfConstrained(ind1) == 0 && limb2->getIfConstrained(ind2) == 0) {
        jac_ind1 = mappedInd2+offset2;
        jac_ind2 = mappedInd1+offset1;
        if (Jacobian(jac_ind1, jac_ind2) == 0 && p != 0) {
            ia[jac_ind1+1]++;
            non_zero_elements.emplace_back(jac_ind1, jac_ind2);
        }
        else if (Jacobian(jac_ind1, jac_ind2) != 0 && Jacobian(jac_ind1, jac_ind2) + p == 0) {
            ia[jac_ind1+1]--;
            // This is expensive, but it doesn't happen that often. Better than using a set
            non_zero_elements.erase(remove(non_zero_elements.begin(),
                                           non_zero_elements.end(),
                                           pair<int, int>(jac_ind1, jac_ind2)), non_zero_elements.end());
        }
        Jacobian(jac_ind1, jac_ind2) += p;
    }
}

void implicitTimeStepper::setZero() {
    baseTimeStepper::setZero();
//    for (int i=0; i < jacobianLen; i++)
//        jacobian[i] = 0;
    non_zero_elements.clear();
    for (int i = 0; i < freeDOF; i++)
        ia[i+1] = 0;
    Jacobian = MatrixXd::Zero(freeDOF,freeDOF);
}


// There are some needless setZeros called but update is called relatively sparsely, so it's ok
void implicitTimeStepper::update() {
    baseTimeStepper::update();
    ldb = freeDOF;
//    delete [] jacobian;
//    delete [] ipiv;
    delete [] ia;

//    jacobianLen = (2 * kl + ku + 1) * freeDOF;
//    jacobian = new double [jacobianLen]{0};
//    ipiv = new int[freeDOF];
    Jacobian = MatrixXd::Zero(freeDOF,freeDOF);

    ia = new MKL_INT[freeDOF+1]{0};
    ia[0] = 1;

    // TODO: check if this is even necessary later
//    setZero();
}


void implicitTimeStepper::prepSystemForIteration() {
    baseTimeStepper::prepSystemForIteration();
    implicitTimeStepper::setZero();
}


void implicitTimeStepper::pardisoSolver()
{
    int n = freeDOF;

    for (int i = 0; i < n-1; i++) {
        ia[i+2] += ia[i+1];
    }
    for (int i = 0; i < n; i++) {
        ia[i+1]++;
    }

    MKL_INT ja[ia[n]];
    double a[ia[n]];

    int ind = 0;
    sort(non_zero_elements.begin(), non_zero_elements.end());
    for (auto& id : non_zero_elements) {
        ja[ind] = id.second + 1;
        a[ind] = Jacobian(id.first, id.second);
        ind++;
    }

    // Descriptor of main sparse matrix properties
//    MKL_INT nrhs = 1;             /* Number of right hand sides. */
    double b[n], x[n];
    /* Auxiliary variables. */
    double ddum;          /* Double dummy */
    MKL_INT idum;         /* Integer dummy. */

/* -------------------------------------------------------------------- */
/* .. Reordering and Symbolic Factorization. This step also allocates */
/* all memory that is necessary for the factorization. */
/* -------------------------------------------------------------------- */
    phase = 11;
    PARDISO (pt, &maxfct, &mnum, &mtype, &phase,
             &n, a, ia, ja, &idum, &nrhs, iparm, &msglvl, &ddum, &ddum, &error);
    if ( error != 0 )
    {
        printf ("\nERROR during symbolic factorization: " IFORMAT, error);
        exit (1);
    }
    // printf ("\nReordering completed ... ");
    // printf ("\nNumber of nonzeros in factors = " IFORMAT, iparm[17]);
    // printf ("\nNumber of factorization MFLOPS = " IFORMAT, iparm[18]);
/* -------------------------------------------------------------------- */
/* .. Numerical factorization. */
/* -------------------------------------------------------------------- */
    phase = 22;
    PARDISO (pt, &maxfct, &mnum, &mtype, &phase,
             &n, a, ia, ja, &idum, &nrhs, iparm, &msglvl, &ddum, &ddum, &error);
    if ( error != 0 )
    {
        printf ("\nERROR during numerical factorization: " IFORMAT, error);
        exit (2);
    }
    // printf ("\nFactorization completed ... ");
/* -------------------------------------------------------------------- */
/* .. Back substitution and iterative refinement. */
/* -------------------------------------------------------------------- */
    phase = 33;

// descrA.type = SPARSE_MATRIX_TYPE_GENERAL;
// descrA.mode = SPARSE_FILL_MODE_UPPER;
// descrA.diag = SPARSE_DIAG_NON_UNIT;
// mkl_sparse_d_create_csr ( &csrA, SPARSE_INDEX_BASE_ONE, n, n, ia, ia+1, ja, a );

    /* Set right hand side to one. */
    for (int i = 0; i < n; i++ )
    {
        b[i] = Force[i];
    }
//  Loop over 3 solving steps: Ax=b, AHx=b and ATx=b
    PARDISO (pt, &maxfct, &mnum, &mtype, &phase,
             &n, a, ia, ja, &idum, &nrhs, iparm, &msglvl, b, x, &error);
    if ( error != 0 )
    {
        printf ("\nERROR during solution: " IFORMAT, error);
        exit (3);
    }

    // printf ("\nThe solution of the system is: ");
    // for ( j = 0; j < n; j++ )
    // {
    //     printf ("\n x [" IFORMAT "] = % f", j, x[j]);
    // }
    // printf ("\n");

/* -------------------------------------------------------------------- */
/* .. Termination and release of memory. */
/* -------------------------------------------------------------------- */
    phase = -1;           /* Release internal memory. */
    PARDISO (pt, &maxfct, &mnum, &mtype, &phase,
             &n, &ddum, ia, ja, &idum, &nrhs,
             iparm, &msglvl, &ddum, &ddum, &error);

    for (int i = 0; i < n; i++)
    {
        dx[i] = x[i];
        DX[i] = x[i];
    }
}

