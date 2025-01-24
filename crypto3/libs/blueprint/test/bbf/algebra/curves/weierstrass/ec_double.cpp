//---------------------------------------------------------------------------//
// Copyright (c) 2024 Antoine Cyr <antoine.cyr@nil.foundation>
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

#define BOOST_TEST_MODULE bbf_check_mod_p_test

#include <boost/test/unit_test.hpp>
#include <nil/blueprint/bbf/components/algebra/curves/weierstrass/ec_double.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>
#include <nil/blueprint/blueprint/plonk/circuit.hpp>

#include <nil/blueprint/bbf/circuit_builder.hpp>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/random/algebraic_engine.hpp>

using namespace nil;
using namespace nil::blueprint;

template<typename BlueprintFieldType, typename NonNativeFieldType, std::size_t num_chunks,
    std::size_t bit_size_chunk>
void test_ec_double(
    const std::vector<typename BlueprintFieldType::value_type>& public_input) {
    using FieldType = BlueprintFieldType;
    using TYPE = typename FieldType::value_type;
    using NON_NATIVE_TYPE = typename NonNativeFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;
    using non_native_integral_type = typename BlueprintFieldType::integral_type;


    non_native_integral_type pow = 1;
    NON_NATIVE_TYPE xQ = 0, yQ = 0;

    for (std::size_t i = 0; i < num_chunks; ++i) {
        xQ += non_native_integral_type(integral_type(public_input[i].data)) * pow;
        yQ += non_native_integral_type(integral_type(public_input[i + num_chunks].data)) * pow;
        pow <<= bit_size_chunk;
    }


    NON_NATIVE_TYPE lambda =
        (yQ == 0)
        ? 0
        : 3 * xQ * xQ *
        ((2 * yQ).inversed()),  // if yQ = 0, lambda = 0
        z = (yQ == 0) ? 0 : yQ.inversed(),                       // if yQ = 0, z = 0
        expected_xR = lambda * lambda - 2 * xQ, expected_yR = lambda * (xQ - expected_xR) - yQ;


    auto assign_and_check = [&](auto& B, auto& raw_input) {
        raw_input.xQ =
            std::vector<TYPE>(public_input.begin(), public_input.begin() + num_chunks);
        raw_input.yQ =
            std::vector<TYPE>(public_input.begin() + num_chunks, public_input.begin() + 2 * num_chunks);
        raw_input.p = std::vector<TYPE>(public_input.begin() + 2 * num_chunks,
            public_input.begin() + 3 * num_chunks);
        raw_input.pp = std::vector<TYPE>(public_input.begin() + 3 * num_chunks,
            public_input.begin() + 4 * num_chunks);
        raw_input.zero = public_input[4 * num_chunks];

        auto [at, A, desc] = B.assign(raw_input);
        bool pass = B.is_satisfied(at);
        std::cout << "Is_satisfied = " << pass << std::endl;

        assert(pass == true);
        non_native_integral_type xR = 0;
        non_native_integral_type yR = 0;
        pow = 1;
        for (std::size_t i = 0; i < num_chunks; i++) {
            xR += non_native_integral_type(integral_type(A.res_xR[i].data)) * pow;
            yR += non_native_integral_type(integral_type(A.res_yR[i].data)) * pow;
            pow <<= bit_size_chunk;
        }
        //#ifdef BLUEPRINT_PLONK_PROFILING_ENABLED
        std::cout << "Expected xR - yR: " << std::dec << expected_xR.data << " - " << expected_yR.data << std::endl;
        std::cout << "Real res xR - yR:  " << std::dec << xR << " - " << yR << std::endl;
        //#endif
        assert(xR == expected_xR.data);
        assert(yR == expected_yR.data);
        };

    if constexpr (std::is_same_v<NonNativeFieldType,
        crypto3::algebra::curves::pallas::base_field_type>) {
        typename bbf::components::pallas_ec_double<
            FieldType, bbf::GenerationStage::ASSIGNMENT>::raw_input_type raw_input;

        auto B =
            bbf::circuit_builder<FieldType, bbf::components::pallas_ec_double,
            std::size_t, std::size_t>(num_chunks, bit_size_chunk);

        assign_and_check(B, raw_input);
    }
    else if constexpr (std::is_same_v<
        NonNativeFieldType,
        crypto3::algebra::curves::vesta::base_field_type>) {
        typename bbf::components::vesta_ec_double<
            FieldType, bbf::GenerationStage::ASSIGNMENT>::raw_input_type raw_input;
        auto B =
            bbf::circuit_builder<FieldType, bbf::components::vesta_ec_double,
            std::size_t, std::size_t>(num_chunks, bit_size_chunk);

        assign_and_check(B, raw_input);
    }
}

template<typename BlueprintFieldType, typename Curve, std::size_t num_chunks,
    std::size_t bit_size_chunk, std::size_t RandomTestsAmount>
void ec_double_tests() {
    using NonNativeFieldType = typename Curve::base_field_type;
    using value_type = typename BlueprintFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;
    using foreign_value_type = typename NonNativeFieldType::value_type;
    using ec_point_value_type = typename Curve::template g1_type<nil::crypto3::algebra::curves::coordinates::affine>::value_type;

    typedef nil::crypto3::multiprecision::big_uint<2 * NonNativeFieldType::modulus_bits>
        extended_integral_type;

    static boost::random::mt19937 seed_seq;
    static nil::crypto3::random::algebraic_engine<BlueprintFieldType> generate_random(
        seed_seq);
    boost::random::uniform_int_distribution<> t_dist(0, 1);


    extended_integral_type mask = (extended_integral_type(1) << bit_size_chunk) - 1;

    for (std::size_t i = 0; i < RandomTestsAmount; i++) {
        std::vector<typename BlueprintFieldType::value_type> public_input;

        extended_integral_type extended_base = 1,
            ext_pow = extended_base << (num_chunks * bit_size_chunk),
            p = NonNativeFieldType::modulus,
            pp = ext_pow - p;

        value_type d = generate_random();
        ec_point_value_type Q = ec_point_value_type::one();
        Q = Q * d;

        public_input.resize(4 * num_chunks + 1);
        integral_type xQ = integral_type(Q.X.data);
        integral_type yQ = integral_type(Q.Y.data);
        for (std::size_t j = 0; j < num_chunks; j++) {
            public_input[j] = value_type(xQ & mask);
            xQ >>= bit_size_chunk;

            public_input[1 * num_chunks + j] = value_type(yQ & mask);
            yQ >>= bit_size_chunk;

            public_input[2 * num_chunks + j] = value_type(p & mask);
            p >>= bit_size_chunk;

            public_input[3 * num_chunks + j] = value_type(pp & mask);
            pp >>= bit_size_chunk;
        }
        public_input.push_back(value_type(0));  // the zero

        test_ec_double<BlueprintFieldType, NonNativeFieldType, num_chunks,
            bit_size_chunk>(public_input);
    }
}

constexpr static const std::size_t random_tests_amount = 10;

BOOST_AUTO_TEST_SUITE(blueprint_plonk_test_suite)

BOOST_AUTO_TEST_CASE(blueprint_plonk_bbf_ec_double_test) {
    using pallas = typename crypto3::algebra::curves::pallas;
    using vesta = typename crypto3::algebra::curves::vesta;

    ec_double_tests<pallas::base_field_type, vesta, 8, 32,
        random_tests_amount>();

    ec_double_tests<pallas::base_field_type, vesta, 4, 65, random_tests_amount>();

    ec_double_tests<vesta::base_field_type, pallas, 4, 65,
        random_tests_amount>();

    ec_double_tests<vesta::base_field_type, pallas, 12, 22,
        random_tests_amount>();

}

BOOST_AUTO_TEST_SUITE_END()
