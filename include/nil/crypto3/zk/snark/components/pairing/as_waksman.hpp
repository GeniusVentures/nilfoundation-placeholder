//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//
// @file Declaration of interfaces for pairing precomputation components.
//
// The components verify correct precomputation of values for the G1 and G2 variables.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_AS_WAKSMAN_HPP
#define CRYPTO3_AS_WAKSMAN_HPP

#include <memory>

#include <nil/crypto3/algebra/algorithms/pairing.hpp>

#include <nil/crypto3/zk/snark/components/curves/weierstrass_g1_component.hpp>
#include <nil/crypto3/zk/snark/components/curves/weierstrass_g2_component.hpp>
#include <nil/crypto3/zk/snark/components/pairing/pairing_params.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {
                namespace components {

                    /**************************** G1 Precomputation ******************************/

                    /**
                     * Not a component. It only holds values.
                     */
                    template<typename CurveType>
                    class G1_precomputation {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        std::shared_ptr<G1_variable<CurveType>> P;
                        std::shared_ptr<Fqe_variable<CurveType>> PY_twist_squared;

                        G1_precomputation(){
                            // will be filled in precompute_G1_component, so do nothing here
                        }
                        
                        G1_precomputation(blueprint<FieldType> &bp,
                                          const typename other_curve<CurveType>::g1_type::value_type &P_val) {
                            typename other_curve<CurveType>::g1_type::value_type P_val_copy = P_val.to_affine_coordinates();
                            P.reset(new G1_variable<CurveType>(bp, P_val_copy));
                            PY_twist_squared.reset(new Fqe_variable<CurveType>(
                                bp, P_val_copy.Y() * other_curve<CurveType>::g2_type::value_type::twist.squared()));
                        }
                    };

                    /**
                     * Gadget that verifies correct precomputation of the G1 variable.
                     */
                    template<typename CurveType>
                    class precompute_G1_component : public component<typename CurveType::scalar_field_type> {
                        using curve_type = CurveType;
                    public:
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        G1_precomputation<CurveType> &precomp;    // must be a reference.

                        /* two possible pre-computations one for mnt4 and one for mnt6 */
                        template<typename FieldType>
                        precompute_G1_component(
                            blueprint<FieldType> &bp,
                            const G1_variable<CurveType> &P,
                            G1_precomputation<CurveType> &precomp,    // will allocate this inside
                            const typename std::enable_if<
                                other_curve<CurveType>::pairing_policy::Fqk_type::arity == 4,
                                typename FieldType::value_type>::type & = typename FieldType::value_type()) :
                            component<FieldType>(bp),
                            precomp(precomp) {

                            blueprint_linear_combination<FieldType> c0, c1;
                            c0.assign(bp, P.Y * ((typename curve_type::pairing_policy().twist).squared().c0));
                            // must be
                            //c0.assign(bp, P.Y * ((typename curve_type::pairing_policy::twist).squared().c0));
                            // when constexpr ready
                            c1.assign(bp, P.Y * ((typename curve_type::pairing_policy().twist).squared().c1));
                            // must be
                            //c1.assign(bp, P.Y * ((typename curve_type::pairing_policy::twist).squared().c1));
                            // when constexpr ready

                            precomp.P.reset(new G1_variable<CurveType>(P));
                            precomp.PY_twist_squared.reset(new Fqe_variable<CurveType>(bp, c0, c1));
                        }

                        template<typename FieldType>
                        precompute_G1_component(
                            blueprint<FieldType> &bp,
                            const G1_variable<CurveType> &P,
                            G1_precomputation<CurveType> &precomp,    // will allocate this inside
                            const typename std::enable_if<
                                other_curve<CurveType>::pairing_policy::Fqk_type::arity == 6,
                                typename FieldType::value_type>::type & = typename FieldType::value_type()) :
                            component<FieldType>(bp),
                            precomp(precomp) {

                            blueprint_linear_combination<FieldType> c0, c1, c2;
                            c0.assign(bp, P.Y * ((typename curve_type::pairing_policy().twist).squared().c0));
                            c1.assign(bp, P.Y * ((typename curve_type::pairing_policy().twist).squared().c1));
                            c2.assign(bp, P.Y * ((typename curve_type::pairing_policy().twist).squared().c2));
                            // must be
                            //c0.assign(bp, P.Y * ((typename curve_type::pairing_policy::twist).squared().c0));
                            //c1.assign(bp, P.Y * ((typename curve_type::pairing_policy::twist).squared().c1));
                            //c2.assign(bp, P.Y * ((typename curve_type::pairing_policy::twist).squared().c2));
                            // when constexpr ready


                            precomp.P.reset(new G1_variable<CurveType>(P));
                            precomp.PY_twist_squared.reset(new Fqe_variable<CurveType>(bp, c0, c1, c2));
                        }

                        void generate_r1cs_constraints() {
                            /* the same for neither CurveType = mnt4 nor CurveType = mnt6 */
                        }

                        void generate_r1cs_witness() {
                            precomp.PY_twist_squared->evaluate(); /* the same for both CurveType = mnt4 and CurveType = mnt6 */
                        }
                    };

                    /**************************** G2 Precomputation ******************************/

                    /**
                     * Not a component. It only holds values.
                     */
                    template<typename CurveType>
                    class precompute_G2_component_coeffs {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        std::shared_ptr<Fqe_variable<CurveType>> RX;
                        std::shared_ptr<Fqe_variable<CurveType>> RY;
                        std::shared_ptr<Fqe_variable<CurveType>> gamma;
                        std::shared_ptr<Fqe_variable<CurveType>> gamma_X;

                        precompute_G2_component_coeffs() {
                            // we will be filled in precomputed case of precompute_G2_component, so do nothing here
                        }

                        precompute_G2_component_coeffs(blueprint<FieldType> &bp){
                            RX.reset(new Fqe_variable<CurveType>(bp));
                            RY.reset(new Fqe_variable<CurveType>(bp));
                            gamma.reset(new Fqe_variable<CurveType>(bp));
                            gamma_X.reset(new Fqe_variable<CurveType>(bp));
                        }

                        precompute_G2_component_coeffs(blueprint<FieldType> &bp, const G2_variable<CurveType> &Q){
                            RX.reset(new Fqe_variable<CurveType>(*(Q.X)));
                            RY.reset(new Fqe_variable<CurveType>(*(Q.Y)));
                            gamma.reset(new Fqe_variable<CurveType>(bp));
                            gamma_X.reset(new Fqe_variable<CurveType>(bp));
                        }
                    };

                    /**
                     * Not a component. It only holds values.
                     */
                    template<typename CurveType>
                    class G2_precomputation {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        std::shared_ptr<G2_variable<CurveType>> Q;

                        std::vector<std::shared_ptr<precompute_G2_component_coeffs<CurveType>>> coeffs;

                        G2_precomputation(){}
                        G2_precomputation(blueprint<FieldType> &bp,
                                          const typename other_curve<CurveType>::g2_type::value_type &Q_val){
                            Q.reset(new G2_variable<CurveType>(bp, Q_val));
                            const typename other_curve<CurveType>::pairing_policy::affine_ate_G2_precomp native_precomp =
                                affine_ate_precompute_G2<other_curve<CurveType>>(Q_val);

                            coeffs.resize(native_precomp.coeffs.size() +
                                          1);    // the last precomp remains for convenient programming
                            for (std::size_t i = 0; i < native_precomp.coeffs.size(); ++i) {
                                coeffs[i].reset(new precompute_G2_component_coeffs<CurveType>());
                                coeffs[i]->RX.reset(new Fqe_variable<CurveType>(bp, native_precomp.coeffs[i].old_RX));
                                coeffs[i]->RY.reset(new Fqe_variable<CurveType>(bp, native_precomp.coeffs[i].old_RY));
                                coeffs[i]->gamma.reset(new Fqe_variable<CurveType>(bp, native_precomp.coeffs[i].gamma));
                                coeffs[i]->gamma_X.reset(new Fqe_variable<CurveType>(bp, native_precomp.coeffs[i].gamma_X));
                            }
                        }
                    };

                    /**
                     * Technical note:
                     *
                     * QX and QY -- X and Y coordinates of Q
                     *
                     * initialization:
                     * coeffs[0].RX = QX
                     * coeffs[0].RY = QY
                     *
                     * G2_precompute_doubling_step relates coeffs[i] and coeffs[i+1] as follows
                     *
                     * coeffs[i]
                     * gamma = (3 * RX^2 + twist_coeff_a) * (2*RY).inversed()
                     * gamma_X = gamma * RX
                     *
                     * coeffs[i+1]
                     * RX = prev_gamma^2 - (2*prev_RX)
                     * RY = prev_gamma * (prev_RX - RX) - prev_RY
                     */
                    template<typename CurveType>
                    class precompute_G2_component_doubling_step : public component<typename CurveType::scalar_field_type> {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        precompute_G2_component_coeffs<CurveType> cur;
                        precompute_G2_component_coeffs<CurveType> next;

                        std::shared_ptr<Fqe_variable<CurveType>> RXsquared;
                        std::shared_ptr<Fqe_sqr_component<CurveType>> compute_RXsquared;
                        std::shared_ptr<Fqe_variable<CurveType>> three_RXsquared_plus_a;
                        std::shared_ptr<Fqe_variable<CurveType>> two_RY;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_gamma;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_gamma_X;

                        std::shared_ptr<Fqe_variable<CurveType>> next_RX_plus_two_RX;
                        std::shared_ptr<Fqe_sqr_component<CurveType>> compute_next_RX;

                        std::shared_ptr<Fqe_variable<CurveType>> RX_minus_next_RX;
                        std::shared_ptr<Fqe_variable<CurveType>> RY_plus_next_RY;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_next_RY;

                        precompute_G2_component_doubling_step(blueprint<FieldType> &bp,
                                                              const precompute_G2_component_coeffs<CurveType> &cur,
                                                              const precompute_G2_component_coeffs<CurveType> &next):
                            component<FieldType>(bp),
                            cur(cur), next(next) {
                            RXsquared.reset(new Fqe_variable<CurveType>(bp));
                            compute_RXsquared.reset(new Fqe_sqr_component<CurveType>(bp, *(cur.RX), *RXsquared));
                            three_RXsquared_plus_a.reset(new Fqe_variable<CurveType>(
                                (*RXsquared) * typename FieldType::value_type(0x03) + other_curve<CurveType>::g2_type::a));
                            two_RY.reset(new Fqe_variable<CurveType>(*(cur.RY) * typename FieldType::value_type(0x02)));

                            compute_gamma.reset(
                                new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *two_RY, *three_RXsquared_plus_a));
                            compute_gamma_X.reset(
                                new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *(cur.RX), *(cur.gamma_X)));

                            next_RX_plus_two_RX.reset(
                                new Fqe_variable<CurveType>(*(next.RX) + *(cur.RX) * typename FieldType::value_type(0x02)));
                            compute_next_RX.reset(new Fqe_sqr_component<CurveType>(bp, *(cur.gamma), *next_RX_plus_two_RX));

                            RX_minus_next_RX.reset(
                                new Fqe_variable<CurveType>(*(cur.RX) + *(next.RX) * (-FieldType::value_type::one())));
                            RY_plus_next_RY.reset(new Fqe_variable<CurveType>(*(cur.RY) + *(next.RY)));
                            compute_next_RY.reset(
                                new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *RX_minus_next_RX, *RY_plus_next_RY));
                        }

                        void generate_r1cs_constraints(){
                            compute_RXsquared->generate_r1cs_constraints();
                            compute_gamma->generate_r1cs_constraints();
                            compute_gamma_X->generate_r1cs_constraints();
                            compute_next_RX->generate_r1cs_constraints();
                            compute_next_RY->generate_r1cs_constraints();
                        }

                        void generate_r1cs_witness(){
                            compute_RXsquared->generate_r1cs_witness();
                            two_RY->evaluate();
                            three_RXsquared_plus_a->evaluate();

                            const fqe_type three_RXsquared_plus_a_val = three_RXsquared_plus_a->get_element();
                            const fqe_type two_RY_val = two_RY->get_element();
                            const fqe_type gamma_val = three_RXsquared_plus_a_val * two_RY_val.inversed();
                            cur.gamma->generate_r1cs_witness(gamma_val);

                            compute_gamma->generate_r1cs_witness();
                            compute_gamma_X->generate_r1cs_witness();

                            const fqe_type RX_val = cur.RX->get_element();
                            const fqe_type RY_val = cur.RY->get_element();
                            const fqe_type next_RX_val = gamma_val.squared() - RX_val - RX_val;
                            const fqe_type next_RY_val = gamma_val * (RX_val - next_RX_val) - RY_val;

                            next.RX->generate_r1cs_witness(next_RX_val);
                            next.RY->generate_r1cs_witness(next_RY_val);

                            RX_minus_next_RX->evaluate();
                            RY_plus_next_RY->evaluate();

                            compute_next_RX->generate_r1cs_witness();
                            compute_next_RY->generate_r1cs_witness();
                        }
                    };

                    /**
                     * Technical note:
                     *
                     * G2_precompute_addition_step relates coeffs[i] and coeffs[i+1] as follows
                     *
                     * coeffs[i]
                     * gamma = (RY - QY) * (RX - QX).inversed()
                     * gamma_X = gamma * QX
                     *
                     * coeffs[i+1]
                     * RX = prev_gamma^2 + (prev_RX + QX)
                     * RY = prev_gamma * (prev_RX - RX) - prev_RY
                     *
                     * (where prev_ in [i+1] refer to things from [i])
                     *
                     * If invert_Q is set to true: use -QY in place of QY everywhere above.
                     */
                    template<typename CurveType>
                    class precompute_G2_component_addition_step : public component<typename CurveType::scalar_field_type> {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        bool invert_Q;
                        precompute_G2_component_coeffs<CurveType> cur;
                        precompute_G2_component_coeffs<CurveType> next;
                        G2_variable<CurveType> Q;

                        std::shared_ptr<Fqe_variable<CurveType>> RY_minus_QY;
                        std::shared_ptr<Fqe_variable<CurveType>> RX_minus_QX;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_gamma;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_gamma_X;

                        std::shared_ptr<Fqe_variable<CurveType>> next_RX_plus_RX_plus_QX;
                        std::shared_ptr<Fqe_sqr_component<CurveType>> compute_next_RX;

                        std::shared_ptr<Fqe_variable<CurveType>> RX_minus_next_RX;
                        std::shared_ptr<Fqe_variable<CurveType>> RY_plus_next_RY;
                        std::shared_ptr<Fqe_mul_component<CurveType>> compute_next_RY;

                        precompute_G2_component_addition_step(blueprint<FieldType> &bp,
                                                              const bool invert_Q,
                                                              const precompute_G2_component_coeffs<CurveType> &cur,
                                                              const precompute_G2_component_coeffs<CurveType> &next,
                                                              const G2_variable<CurveType> &Q) :
                            component<FieldType>(bp),
                            invert_Q(invert_Q), cur(cur), next(next), Q(Q) {
                            RY_minus_QY.reset(
                                new Fqe_variable<CurveType>(*(cur.RY) + *(Q.Y) * (!invert_Q ? -FieldType::value_type::one() :
                                                                                              FieldType::value_type::one())));

                            RX_minus_QX.reset(
                                new Fqe_variable<CurveType>(*(cur.RX) + *(Q.X) * (-FieldType::value_type::one())));
                            compute_gamma.reset(new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *RX_minus_QX, *RY_minus_QY));
                            compute_gamma_X.reset(new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *(Q.X), *(cur.gamma_X)));

                            next_RX_plus_RX_plus_QX.reset(new Fqe_variable<CurveType>(*(next.RX) + *(cur.RX) + *(Q.X)));
                            compute_next_RX.reset(new Fqe_sqr_component<CurveType>(bp, *(cur.gamma), *next_RX_plus_RX_plus_QX));

                            RX_minus_next_RX.reset(
                                new Fqe_variable<CurveType>(*(cur.RX) + *(next.RX) * (-FieldType::value_type::one())));
                            RY_plus_next_RY.reset(new Fqe_variable<CurveType>(*(cur.RY) + *(next.RY)));
                            compute_next_RY.reset(
                                new Fqe_mul_component<CurveType>(bp, *(cur.gamma), *RX_minus_next_RX, *RY_plus_next_RY));
                        }

                        void generate_r1cs_constraints() {
                            compute_gamma->generate_r1cs_constraints();
                            compute_gamma_X->generate_r1cs_constraints();
                            compute_next_RX->generate_r1cs_constraints();
                            compute_next_RY->generate_r1cs_constraints();
                        }

                        void generate_r1cs_witness() {
                            RY_minus_QY->evaluate();
                            RX_minus_QX->evaluate();

                            const fqe_type RY_minus_QY_val = RY_minus_QY->get_element();
                            const fqe_type RX_minus_QX_val = RX_minus_QX->get_element();
                            const fqe_type gamma_val = RY_minus_QY_val * RX_minus_QX_val.inversed();
                            cur.gamma->generate_r1cs_witness(gamma_val);

                            compute_gamma->generate_r1cs_witness();
                            compute_gamma_X->generate_r1cs_witness();

                            const fqe_type RX_val = cur.RX->get_element();
                            const fqe_type RY_val = cur.RY->get_element();
                            const fqe_type QX_val = Q.X->get_element();
                            const fqe_type next_RX_val = gamma_val.squared() - RX_val - QX_val;
                            const fqe_type next_RY_val = gamma_val * (RX_val - next_RX_val) - RY_val;

                            next.RX->generate_r1cs_witness(next_RX_val);
                            next.RY->generate_r1cs_witness(next_RY_val);

                            next_RX_plus_RX_plus_QX->evaluate();
                            RX_minus_next_RX->evaluate();
                            RY_plus_next_RY->evaluate();

                            compute_next_RX->generate_r1cs_witness();
                            compute_next_RY->generate_r1cs_witness();
                        }
                    };

                    /**
                     * Gadget that verifies correct precomputation of the G2 variable.
                     */
                    template<typename CurveType>
                    class precompute_G2_component : public component<typename CurveType::scalar_field_type> {
                    public:
                        typedef typename CurveType::pairing_policy::Fp_type FieldType;
                        using fqe_type = typename other_curve<CurveType>::pairing_policy::Fqe_type;
                        using fqk_type = typename other_curve<CurveType>::pairing_policy::Fqk_type;

                        std::vector<std::shared_ptr<precompute_G2_component_addition_step<CurveType>>> addition_steps;
                        std::vector<std::shared_ptr<precompute_G2_component_doubling_step<CurveType>>> doubling_steps;

                        std::size_t add_count;
                        std::size_t dbl_count;

                        G2_precomputation<CurveType> &precomp;    // important to have a reference here

                        precompute_G2_component(blueprint<FieldType> &bp,
                                                const G2_variable<CurveType> &Q,
                                                G2_precomputation<CurveType> &precomp):
                            component<FieldType>(bp),
                            precomp(precomp) {
                            precomp.Q.reset(new G2_variable<CurveType>(Q));

                            const auto &loop_count = pairing_selector<CurveType>::pairing_loop_count;
                            std::size_t coeff_count =
                                1;    // the last RX/RY are unused in Miller loop, but will need to get allocated somehow
                            this->add_count = 0;
                            this->dbl_count = 0;

                            bool found_nonzero = false;
                            std::vector<long> NAF = find_wnaf(1, loop_count);
                            for (long i = NAF.size() - 1; i >= 0; --i) {
                                if (!found_nonzero) {
                                    /* this skips the MSB itself */
                                    found_nonzero |= (NAF[i] != 0);
                                    continue;
                                }

                                ++dbl_count;
                                ++coeff_count;

                                if (NAF[i] != 0) {
                                    ++add_count;
                                    ++coeff_count;
                                }
                            }

                            precomp.coeffs.resize(coeff_count);
                            addition_steps.resize(add_count);
                            doubling_steps.resize(dbl_count);

                            precomp.coeffs[0].reset(new precompute_G2_component_coeffs<CurveType>(bp, Q));
                            for (std::size_t i = 1; i < coeff_count; ++i) {
                                precomp.coeffs[i].reset(new precompute_G2_component_coeffs<CurveType>(bp));
                            }

                            std::size_t add_id = 0;
                            std::size_t dbl_id = 0;
                            std::size_t coeff_id = 0;

                            found_nonzero = false;
                            for (long i = NAF.size() - 1; i >= 0; --i) {
                                if (!found_nonzero) {
                                    /* this skips the MSB itself */
                                    found_nonzero |= (NAF[i] != 0);
                                    continue;
                                }

                                doubling_steps[dbl_id].reset(new precompute_G2_component_doubling_step<CurveType>(
                                    bp, *(precomp.coeffs[coeff_id]), *(precomp.coeffs[coeff_id + 1])));
                                ++dbl_id;
                                ++coeff_id;

                                if (NAF[i] != 0) {
                                    addition_steps[add_id].reset(new precompute_G2_component_addition_step<CurveType>(
                                        bp, NAF[i] < 0, *(precomp.coeffs[coeff_id]), *(precomp.coeffs[coeff_id + 1]), Q));
                                    ++add_id;
                                    ++coeff_id;
                                }
                            }
                        }

                        void generate_r1cs_constraints(){
                            for (std::size_t i = 0; i < dbl_count; ++i) {
                                doubling_steps[i]->generate_r1cs_constraints();
                            }

                            for (std::size_t i = 0; i < add_count; ++i) {
                                addition_steps[i]->generate_r1cs_constraints();
                            }
                        }

                        void generate_r1cs_witness(){
                            precomp.coeffs[0]->RX->generate_r1cs_witness(precomp.Q->X->get_element());
                            precomp.coeffs[0]->RY->generate_r1cs_witness(precomp.Q->Y->get_element());

                            const auto &loop_count = pairing_selector<CurveType>::pairing_loop_count;

                            std::size_t add_id = 0;
                            std::size_t dbl_id = 0;

                            bool found_nonzero = false;
                            std::vector<long> NAF = find_wnaf(1, loop_count);
                            for (long i = NAF.size() - 1; i >= 0; --i) {
                                if (!found_nonzero) {
                                    /* this skips the MSB itself */
                                    found_nonzero |= (NAF[i] != 0);
                                    continue;
                                }

                                doubling_steps[dbl_id]->generate_r1cs_witness();
                                ++dbl_id;

                                if (NAF[i] != 0) {
                                    addition_steps[add_id]->generate_r1cs_witness();
                                    ++add_id;
                                }
                            }
                        }
                    };

                }    // namespace components
            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_AS_WAKSMAN_HPP
