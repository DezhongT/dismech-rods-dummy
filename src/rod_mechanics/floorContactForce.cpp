#include "floorContactForce.h"

floorContactForce::floorContactForce(vector<shared_ptr<elasticRod>> m_limbs, shared_ptr<timeStepper> m_stepper,
                                     double m_floor_delta, double m_floor_slipTol, double m_floor_mu) {
    limbs = m_limbs;
    stepper = m_stepper;

    delta = m_floor_delta;
    slipTol = m_floor_slipTol;

    K1 = 15 / delta;
    K2 = 15 / slipTol;

//    mu = m_floor_mu;
    mu = 0.1;

    floor_z = -0.15;

    contact_stiffness = 100000;
//    contact_stiffness = 1000000;

    contact_input[1] = K1;

//    fric_jacobian_input[5] = mu;
//    fric_jacobian_input[6] = rod->dt;
//    fric_jacobian_input[7] = K2;
//
//    sym_eqs = make_shared<symbolicEquations>();
//    sym_eqs->generateFloorFrictionJacobianFunctions();
}

floorContactForce::~floorContactForce() {
    ;
}

void floorContactForce::updateMu(double m_mu) {
    mu = m_mu;
    fric_jacobian_input[5] = mu;
}


void floorContactForce::computeFf(bool fric_off) {
    double dist;
    double f;
    int limb_idx = 0;
    int ind;
    double v;  // exp(K1(floor_z - \Delta))
    for (const auto& limb : limbs) {
        for (int i=0; i < limb->nv; i++) {
            ind = 4 * i + 2;
            if (limb->isDOFJoint[ind] == 1) continue;

//            dist = limb->x[ind] - floor_z;
//            if (dist > delta) continue;
            dist = floor_z - limb->x[ind];
            if (-dist > delta) continue;

            v = exp(K1 * dist);

            f = (-2 * v * log(v + 1)) / (K1 * (v + 1));
            f *= contact_stiffness;
//            cout << dist << " " << f << " " << v << endl;
            stepper->addForce(ind, f, limb_idx);

        }
        limb_idx++;
    }
}


void floorContactForce::computeFfJf(bool fric_off) {
    double dist;
    double J;
    int limb_idx = 0;
    int ind;
    double v;  // exp(K1(floor_z - \Delta))
    for (const auto& limb : limbs) {
        for (int i = 0; i < limb->nv; i++) {
            ind = 4 * i + 2;
            if (limb->isDOFJoint[ind] == 1) continue;

//            dist = limb->x[ind] - floor_z;
//            if (dist > delta) continue;

            dist = floor_z - limb->x[ind];
            if (-dist > delta) continue;

            v = exp(K1 * dist);

            J = (2*v * log(v + 1) + v) / pow(v + 1, 2);
            J *= contact_stiffness;
            stepper->addJacobian(ind, ind, J, limb_idx);
        }
        limb_idx++;
    }
}


//void floorContactForce::computeFf(bool fric_off) {
//    for (int i = 0; i < rod->nv; i++) {
////        z = rod->getVertex(i)(2);
////        if (z > delta) {
////            continue;
////        }
////        contact_input[0] = z;
////        if (z < -delta) {
////            sym_eqs->floor_con_pen_gradient_func.call(&fn, contact_input.data());
////        }
////        else {
////            sym_eqs->floor_con_gradient_func.call(&fn, contact_input.data());
////        }
////
////        fn *= contact_stiffness;
////        stepper->addForce(4*i+2, fn);
//
//        fn = rod->massArray[4*i] * 9.8;
//
//        if (!fric_off) {
//            computeFriction(i);
//            stepper->addForce(4*i, ffr(0));
//            stepper->addForce(4*i+1, ffr(1));
//        }
//    }
//}


//void floorContactForce::computeFfJf(bool fric_off) {
//    for (int i = 0; i < rod->nv; i++) {
////        z = rod->getVertex(i)(2);
////        if (z > delta) {
////            continue;
////        }
////        contact_input[0] = z;
////        if (z < -delta) {
////            sym_eqs->floor_con_pen_gradient_func.call(&fn, contact_input.data());
////            sym_eqs->floor_con_pen_hessian_func.call(&jfn, contact_input.data());
////        }
////        else {
////            sym_eqs->floor_con_gradient_func.call(&fn, contact_input.data());
////            sym_eqs->floor_con_hessian_func.call(&jfn, contact_input.data());
////        }
////        fn *= contact_stiffness;
////        jfn *= contact_stiffness;
////        stepper->addForce(4*i+2, fn);
////        stepper->addJacobian(4*i*2, 4*i*2, jfn);
//
//        fn = rod->massArray[4*i] * 9.8;
//
//        if (!fric_off) {
//            computeFriction(i);
//            if (fric_jaco_type == 0) {
//                continue;
//            }
//            stepper->addForce(4*i, ffr(0));
//            stepper->addForce(4*i+1, ffr(1));
//
//            prepFrictionJacobianInput(i);
//
//            if (fric_jaco_type == 1) {
//                sym_eqs->floor_friction_partials_gamma1_dfr_dx_func.call(friction_partials_dfr_dx.data(), fric_jacobian_input.data());
//                sym_eqs->floor_friction_partials_gamma1_dfr_dfn_func.call(friction_partials_dfr_dfn.data(), fric_jacobian_input.data());
//            }
//            else {
//                sym_eqs->floor_friction_partials_dfr_dx_func.call(friction_partials_dfr_dx.data(), fric_jacobian_input.data());
//                sym_eqs->floor_friction_partials_dfr_dfn_func.call(friction_partials_dfr_dfn.data(), fric_jacobian_input.data());
//            }
//
//            // TODO: not utilizing full friction jacobian
//            // dfrx/dx
//            stepper->addJacobian(4*i, 4*i, friction_partials_dfr_dx(0, 0));
//            stepper->addJacobian(4*i+1, 4*i, friction_partials_dfr_dx(0, 1));
////            stepper->addJacobian(4*i+1, 4*i, friction_partials_dfr_dx(1, 0));
//            // dfry/dy
//            stepper->addJacobian(4*i+1, 4*i+1, friction_partials_dfr_dx(1, 1));
//            stepper->addJacobian(4*i, 4*i+1, friction_partials_dfr_dx(1, 0));
////            stepper->addJacobian(4*i, 4*i+1, friction_partials_dfr_dx(0, 1));
//
//
//
//            // dfrx/dfn * dfn/dz = dfrx/dz
////            stepper->addJacobian(4*i+2, 4*i, friction_partials_dfr_dfn(0) * jfn);
//            // dfry/dfn * dfn/dz = dfry/dz
////            stepper->addJacobian(4*i+2, 4*i+1, friction_partials_dfr_dfn(1) * jfn);
//        }
//    }
//}

//void floorContactForce::computeFriction(const int& edge) {
//    Vector3d curr_node, pre_node;
//    Vector2d v, v_hat;
//    double v_n, gamma;
//    curr_node = rod->getVertex(edge);
//    pre_node = rod->getPreVertex(edge);
//
//    v = (curr_node(seq(0, 1)) - pre_node(seq(0, 1))) / rod->dt;
//    v_n = v.norm();
//    v_hat = v / v_n;
//
//    if (v_n == 0) {
//        fric_jaco_type = 0;
//        ffr.setZero();
//        return;
//    }
//    else if (v_n > slipTol) {
//        fric_jaco_type = 1;
//        gamma = 1;
//    }
//    else {
//        fric_jaco_type = 2;
//        gamma = 2 / (1 + exp(-K2 * v_n)) - 1;
//    }
//    ffr = gamma * mu * abs(fn) * v_hat;
//}
//

//void floorContactForce::prepFrictionJacobianInput(const int& edge) {
//    Vector3d curr_node = rod->getVertex(edge);
//    Vector3d pre_node = rod->getPreVertex(edge);
//    fric_jacobian_input(0) = curr_node(0);
//    fric_jacobian_input(1) = curr_node(1);
//    fric_jacobian_input(2) = pre_node(0);
//    fric_jacobian_input(3) = pre_node(1);
//    fric_jacobian_input(4) = fn;
//}




//void floorContactForce::updateContactStiffness() {
//    double curr_min_z = 1e28;  // arbitrary large number
//    double z;
//    for (int i = 0; i < rod->nv; i++) {
//        z = rod->getVertex(i)(2);
//        if (z < curr_min_z) {
//            curr_min_z = z;
//        }
//    }
//    if (curr_min_z > 0.001) {
//        contact_stiffness *= 0.999;
//    }
//    else if (curr_min_z < -0.004) {
//        contact_stiffness *= 1.01;
//    }
//    else if (curr_min_z < -0.002) {
//        contact_stiffness *= 1.005;
//    }
//    else if (curr_min_z < -0.001) {
//        contact_stiffness *= 1.003;
//    }
//    else if (curr_min_z < 0) {
//        contact_stiffness *= 1.001;
//    }
//}