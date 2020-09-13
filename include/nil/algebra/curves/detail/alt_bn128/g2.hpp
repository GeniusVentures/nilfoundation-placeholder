//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef ALGEBRA_CURVES_ALT_BN128_G2_HPP
#define ALGEBRA_CURVES_ALT_BN128_G2_HPP

#include <nil/algebra/curves/detail/alt_bn128/basic_policy.hpp>
#include <nil/algebra/curves/detail/alt_bn128/g1.hpp>

#include <nil/algebra/fields/fp2.hpp>

#include <nil/algebra/detail/literals.hpp>

namespace nil {
    namespace algebra {
        namespace curves {
            namespace detail {

                template<std::size_t ModulusBits, std::size_t GeneratorBits>
                struct alt_bn128_g2 {

                    using policy_type = alt_bn128_basic_policy<ModulusBits, GeneratorBits>;
                    constexpr static const std::size_t g1_field_bits = policy_type::base_field_bits;
                    typedef typename policy_type::base_field_type::value_type g1_field_type_value;

                    constexpr static const std::size_t g2_field_bits = policy_type::base_field_bits;
                    typedef typename fields::fp2<typename policy_type::base_field_type>::value_type g2_field_type_value;

                    using underlying_field_type_value = g2_field_type_value;

                    underlying_field_type_value p[3];

                    alt_bn128_g2() :
                        alt_bn128_g2(underlying_field_type_value::zero(), underlying_field_type_value::one(),
                                     underlying_field_type_value::zero()) {};
                    // must be
                    // alt_bn128_g2() : alt_bn128_g2(zero_fill[0], zero_fill[1], zero_fill[2]) {};
                    // when constexpr fields will be finished

                    alt_bn128_g2(underlying_field_type_value X, underlying_field_type_value Y,
                                 underlying_field_type_value Z) {
                        p[0] = X;
                        p[1] = Y;
                        p[2] = Z;
                    };

                    static alt_bn128_g2 zero() {
                        return alt_bn128_g2();
                    }

                    static alt_bn128_g2 one() {
                        return alt_bn128_g2(
                            underlying_field_type_value(
                                0x1800DEEF121F1E76426A00665E5C4479674322D4F75EDADD46DEBD5CD992F6ED_cppui254,
                                0x198E9393920D483A7260BFB731FB5D25F1AA493335A9E71297E485B7AEF312C2_cppui254),
                            underlying_field_type_value(
                                0x12C85EA5DB8C6DEB4AAB71808DCB408FE3D1E7690C43D37B4CE6CC0166FA7DAA_cppui254,
                                0x90689D0585FF075EC9E99AD690C3395BC4B313370B38EF355ACDADCD122975B_cppui254),
                            underlying_field_type_value::one());
                        // must be
                        // return alt_bn128_g2(one_fill[0], one_fill[1], one_fill[2]);
                        // when constexpr fields will be finished
                    }

                    bool operator==(const alt_bn128_g2 &other) const {
                        if (this->is_zero()) {
                            return other.is_zero();
                        }

                        if (other.is_zero()) {
                            return false;
                        }

                        /* now neither is O */

                        // using Jacobian coordinates so:
                        // (X1:Y1:Z1) = (X2:Y2:Z2)
                        // iff
                        // X1/Z1^2 == X2/Z2^2 and Y1/Z1^3 == Y2/Z2^3
                        // iff
                        // X1 * Z2^2 == X2 * Z1^2 and Y1 * Z2^3 == Y2 * Z1^3

                        underlying_field_type_value Z1_squared = (this->p[2]).squared();
                        underlying_field_type_value Z2_squared = (other.p[2]).squared();

                        if ((this->p[0] * Z2_squared) != (other.p[0] * Z1_squared)) {
                            return false;
                        }

                        underlying_field_type_value Z1_cubed = (this->p[2]) * Z1_squared;
                        underlying_field_type_value Z2_cubed = (other.p[2]) * Z2_squared;

                        if ((this->p[1] * Z2_cubed) != (other.p[1] * Z1_cubed)) {
                            return false;
                        }

                        return true;
                    }

                    bool operator!=(const alt_bn128_g2 &other) const {
                        return !(operator==(other));
                    }

                    bool is_zero() const {
                        return (this->p[2].is_zero());
                    }

                    alt_bn128_g2 operator-() const {
                        return alt_bn128_g2(this->p[0], -(this->p[1]), this->p[2]);
                    }

                    alt_bn128_g2 operator-(const alt_bn128_g2 &other) const {
                        return (*this) + (-other);
                    }

                    alt_bn128_g2 operator+(const alt_bn128_g2 &other) const {
                        // handle special cases having to do with O
                        if (this->is_zero()) {
                            return other;
                        }

                        if (other.is_zero()) {
                            return *this;
                        }

                        // no need to handle points of order 2,4
                        // (they cannot exist in a prime-order subgroup)

                        // handle double case
                        if (this->operator==(other)) {
                            return this->doubled();
                        }

                        // NOTE: does not handle O and pts of order 2,4
                        // http://www.hyperelliptic.org/EFD/g1p/auto-shortw-projective.html#addition-add-1998-cmo-2

                        underlying_field_type_value Z1Z1 = (this->p[2]).squared();              // Z1Z1 = Z1^2
                        underlying_field_type_value Z2Z2 = (other.p[2]).squared();              // Z2Z2 = Z2^2
                        underlying_field_type_value U1 = (this->p[0]) * Z2Z2;                   // U1 = X1 * Z2Z2
                        underlying_field_type_value U2 = (other.p[0]) * Z1Z1;                   // U2 = X2 * Z1Z1
                        underlying_field_type_value S1 = (this->p[1]) * (other.p[2]) * Z2Z2;    // S1 = Y1 * Z2 * Z2Z2
                        underlying_field_type_value S2 = (other.p[1]) * (this->p[2]) * Z1Z1;    // S2 = Y2 * Z1 * Z1Z1
                        underlying_field_type_value H = U2 - U1;                                // H = U2-U1
                        underlying_field_type_value S2_minus_S1 = S2 - S1;
                        underlying_field_type_value I = H.doubled().squared();             // I = (2 * H)^2
                        underlying_field_type_value J = H * I;                             // J = H * I
                        underlying_field_type_value r = S2_minus_S1.doubled();             // r = 2 * (S2-S1)
                        underlying_field_type_value V = U1 * I;                            // V = U1 * I
                        underlying_field_type_value X3 = r.squared() - J - V.doubled();    // X3 = r^2 - J - 2 * V
                        underlying_field_type_value S1_J = S1 * J;
                        underlying_field_type_value Y3 = r * (V - X3) - S1_J.doubled();    // Y3 = r * (V-X3)-2 S1 J
                        underlying_field_type_value Z3 = ((this->p[2] + other.p[2]).squared() - Z1Z1 - Z2Z2) *
                                                         H;    // Z3 = ((Z1+Z2)^2-Z1Z1-Z2Z2) * H

                        return alt_bn128_g2(X3, Y3, Z3);
                    }

                    alt_bn128_g2 doubled() const {
                        // handle point at infinity
                        if (this->is_zero()) {
                            return (*this);
                        }

                        // NOTE: does not handle O and pts of order 2,4
                        // http://www.hyperelliptic.org/EFD/g1p/auto-shortw-projective.html#doubling-dbl-2007-bl

                        underlying_field_type_value A = (this->p[0]).squared();    // A = X1^2
                        underlying_field_type_value B = (this->p[1]).squared();    // B = Y1^2
                        underlying_field_type_value C = B.squared();               // C = B^2
                        underlying_field_type_value D = (this->p[0] + B).squared() - A - C;
                        D = D + D;                                           // D = 2 * ((X1 + B)^2 - A - C)
                        underlying_field_type_value E = A.doubled() + A;     // E = 3 * A
                        underlying_field_type_value F = E.squared();         // F = E^2
                        underlying_field_type_value X3 = F - D.doubled();    // X3 = F - 2 D
                        underlying_field_type_value eightC = C.doubled().doubled().doubled();
                        underlying_field_type_value Y3 = E * (D - X3) - eightC;    // Y3 = E * (D - X3) - 8 * C
                        underlying_field_type_value Y1Z1 = (this->p[1]) * (this->p[2]);
                        underlying_field_type_value Z3 = Y1Z1 + Y1Z1;    // Z3 = 2 * Y1 * Z1

                        return alt_bn128_g2(X3, Y3, Z3);
                    }

                    alt_bn128_g2 mixed_add(const alt_bn128_g2 &other) const {

                        // handle special cases having to do with O
                        if (this->is_zero()) {
                            return other;
                        }

                        if (other.is_zero()) {
                            return *this;
                        }

                        // no need to handle points of order 2,4
                        // (they cannot exist in a prime-order subgroup)

                        // check for doubling case

                        // using Jacobian coordinates so:
                        // (X1:Y1:Z1) = (X2:Y2:Z2)
                        // iff
                        // X1/Z1^2 == X2/Z2^2 and Y1/Z1^3 == Y2/Z2^3
                        // iff
                        // X1 * Z2^2 == X2 * Z1^2 and Y1 * Z2^3 == Y2 * Z1^3

                        // we know that Z2 = 1

                        const underlying_field_type_value Z1Z1 = (this->p[2]).squared();

                        const underlying_field_type_value &U1 = this->p[0];
                        const underlying_field_type_value U2 = other.p[0] * Z1Z1;

                        const underlying_field_type_value Z1_cubed = (this->p[2]) * Z1Z1;

                        const underlying_field_type_value &S1 = (this->p[1]);              // S1 = Y1 * Z2 * Z2Z2
                        const underlying_field_type_value S2 = (other.p[1]) * Z1_cubed;    // S2 = Y2 * Z1 * Z1Z1

                        if (U1 == U2 && S1 == S2) {
                            // dbl case; nothing of above can be reused
                            return this->doubled();
                        }

                        // NOTE: does not handle O and pts of order 2,4
                        // http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-0.html#addition-madd-2007-bl
                        underlying_field_type_value H = U2 - (this->p[0]);    // H = U2-X1
                        underlying_field_type_value HH = H.squared();         // HH = H&2
                        underlying_field_type_value I = HH + HH;              // I = 4*HH
                        I = I + I;
                        underlying_field_type_value J = H * I;                // J = H*I
                        underlying_field_type_value r = S2 - (this->p[1]);    // r = 2*(S2-Y1)
                        r = r + r;
                        underlying_field_type_value V = (this->p[0]) * I;            // V = X1*I
                        underlying_field_type_value X3 = r.squared() - J - V - V;    // X3 = r^2-J-2*V
                        underlying_field_type_value Y3 = (this->p[1]) * J;           // Y3 = r*(V-X3)-2*Y1*J
                        Y3 = r * (V - X3) - Y3 - Y3;
                        underlying_field_type_value Z3 =
                            ((this->p[2]) + H).squared() - Z1Z1 - HH;    // Z3 = (Z1+H)^2-Z1Z1-HH

                        return alt_bn128_g2(X3, Y3, Z3);
                    }

                    void to_affine_coordinates() {
                        if (this->is_zero()) {
                            this->p[0] = underlying_field_type_value::zero();
                            this->p[1] = underlying_field_type_value::one();
                            this->p[2] = underlying_field_type_value::zero();
                        } else {
                            underlying_field_type_value Z_inv = this->p[2].inversed();
                            underlying_field_type_value Z2_inv = Z_inv.squared();
                            underlying_field_type_value Z3_inv = Z2_inv * Z_inv;
                            this->p[0] = this->p[0] * Z2_inv;
                            this->p[1] = this->p[1] * Z3_inv;
                            this->p[2] = underlying_field_type_value::one();
                        }
                    }

                    void to_special() {
                        this->to_affine_coordinates();
                    }

                    bool is_special() const {
                        return (this->is_zero() || this->p[2] == underlying_field_type_value::one());
                    }

                private:
                    /*alt_bn128_g2 mul_by_q() const {
                        return alt_bn128_g2(twist_mul_by_q_X * (this->p[0]).Frobenius_map(1),
                                            twist_mul_by_q_Y * (this->p[1]).Frobenius_map(1),
                                            (this->p[2]).Frobenius_map(1));
                    }*/

                    /*constexpr static */ const g1_field_type_value a = g1_field_type_value(policy_type::a);
                    /*constexpr static */ const g1_field_type_value b = g1_field_type_value(policy_type::b);

                    /*constexpr static */ const g2_field_type_value twist = g2_field_type_value(0x09, 0x01);

                    /*constexpr static */ const g2_field_type_value twist_coeff_b = b * twist.inversed();

                    /*constexpr static */ const g1_field_type_value twist_mul_by_b_c0 = b * twist.non_residue;
                    /*constexpr static */ const g1_field_type_value twist_mul_by_b_c1 = b * twist.non_residue;
                    /*constexpr static */ const g2_field_type_value twist_mul_by_q_X = g2_field_type_value(
                        {0x2FB347984F7911F74C0BEC3CF559B143B78CC310C2C3330C99E39557176F553D_cppui254,
                         0x16C9E55061EBAE204BA4CC8BD75A079432AE2A1D0B7C9DCE1665D51C640FCBA2_cppui253});
                    /*constexpr static */ const g2_field_type_value twist_mul_by_q_Y = g2_field_type_value(
                        {0x63CF305489AF5DCDC5EC698B6E2F9B9DBAAE0EDA9C95998DC54014671A0135A_cppui251,
                         0x7C03CBCAC41049A0704B5A7EC796F2B21807DC98FA25BD282D37F632623B0E3_cppui251});

                    /*constexpr static const underlying_field_type_value zero_fill = {
                        underlying_field_type_value::zero(), underlying_field_type_value::one(),
                        underlying_field_type_value::zero()};

                    constexpr static const underlying_field_type_value one_fill = {
                        underlying_field_type_value(
                            0x1800DEEF121F1E76426A00665E5C4479674322D4F75EDADD46DEBD5CD992F6ED_cppui254,
                            0x198E9393920D483A7260BFB731FB5D25F1AA493335A9E71297E485B7AEF312C2_cppui254),
                        underlying_field_type_value(
                            0x12C85EA5DB8C6DEB4AAB71808DCB408FE3D1E7690C43D37B4CE6CC0166FA7DAA_cppui254,
                            0x90689D0585FF075EC9E99AD690C3395BC4B313370B38EF355ACDADCD122975B_cppui254),
                        underlying_field_type_value::one()};*/
                };

            }    // namespace detail
        }        // namespace curves
    }            // namespace algebra
}    // namespace nil
#endif    // ALGEBRA_CURVES_ALT_BN128_G2_HPP
