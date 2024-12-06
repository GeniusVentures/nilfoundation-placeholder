//---------------------------------------------------------------------------//
// Copyright (c) 2024 Polina Chernyshova <pockvokhbtra@nil.foundation>
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

#define BOOST_TEST_MODULE bbf_range_check_multi_test

#include <boost/test/unit_test.hpp>
#include <nil/blueprint/bbf/components/hashes/poseidon/plonk/poseidon.hpp>
#include <nil/blueprint/bbf/generic.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>
#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/bbf/components/detail/range_check_multi.hpp>

#include <nil/blueprint/bbf/circuit_builder.hpp>

#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/random/algebraic_engine.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
//#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/curves/pallas.hpp>
//#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>

using namespace nil;
using namespace nil::blueprint;


template <typename BlueprintFieldType, std::size_t num_chunks, std::size_t bit_size_chunk, bool to_pass = true>
void test_range_check(const std::vector<typename BlueprintFieldType::value_type> &public_input){

    using FieldType = BlueprintFieldType;
    typename bbf::components::range_check_multi<FieldType,bbf::GenerationStage::ASSIGNMENT,num_chunks,bit_size_chunk>::raw_input_type raw_input;
    raw_input.state = public_input;
   auto B = bbf::circuit_builder<FieldType, bbf::components::range_check_multi>();

    auto [at, A] = B.assign(raw_input);
    std::cout << "Is_satisfied = " << B.is_satisfied(at) << std::endl;

    if (to_pass) {
        assert(B.is_satisfied(at) == true);
    } else {
        assert(B.is_satisfied(at) == false);
    }
}


template <typename BlueprintFieldType, std::size_t num_chunks, std::size_t bit_size_chunk, std::size_t RandomTestsAmount>
void range_check_tests() {
    using integral_type = typename BlueprintFieldType::integral_type;
    using value_type = typename BlueprintFieldType::value_type;

    static boost::random::mt19937 seed_seq;
    static nil::crypto3::random::algebraic_engine<BlueprintFieldType> generate_random(seed_seq);
    boost::random::uniform_int_distribution<> t_dist(0, 1);
    integral_type mask = (integral_type(1) << bit_size_chunk) - 1;

    for (std::size_t i = 0; i < RandomTestsAmount; i++) {
        std::vector<typename BlueprintFieldType::value_type> public_input;
        for (std::size_t j = 0; j < num_chunks; j++) {
            public_input.push_back(value_type(integral_type(generate_random().data) & mask));
        }
        test_range_check<BlueprintFieldType,num_chunks,bit_size_chunk>(public_input);
    }
}

template <typename BlueprintFieldType, std::size_t num_chunks, std::size_t bit_size_chunk,std::size_t RandomTestsAmount>
void range_check_tests_to_fail() {
    using integral_type = typename BlueprintFieldType::integral_type;
    using value_type = typename BlueprintFieldType::value_type;

    static boost::random::mt19937 seed_seq;
    static nil::crypto3::random::algebraic_engine<BlueprintFieldType> generate_random(seed_seq);
    boost::random::uniform_int_distribution<> t_dist(0, 1);
    integral_type mask = (integral_type(1) << bit_size_chunk) - 1;

    for (std::size_t i = 0; i < RandomTestsAmount; i++) {
        std::size_t for_faulty_chunks = bit_size_chunk % num_chunks + 1;
        integral_type most_significant_bit = integral_type(1) << (bit_size_chunk + i % 2);
        std::vector<typename BlueprintFieldType::value_type> public_input;
        for (std::size_t j = 0; j < num_chunks; j++) {
            if (j % for_faulty_chunks != 0) {
                public_input.push_back(value_type(integral_type(generate_random().data) & mask));
            } else {
                public_input.push_back(value_type(integral_type(generate_random().data) & mask | most_significant_bit));
            }
        }
        test_range_check<BlueprintFieldType,num_chunks,bit_size_chunk,false>(public_input);
    }
}

constexpr static const std::size_t random_tests_amount = 10;

BOOST_AUTO_TEST_SUITE(blueprint_plonk_test_suite)

BOOST_AUTO_TEST_CASE(blueprint_plonk_equality_flag_test) {
    using pallas_field_type = typename crypto3::algebra::curves::pallas::base_field_type;
    using vesta_field_type = typename crypto3::algebra::curves::vesta::base_field_type;

    range_check_tests<pallas_field_type, 8, 32, random_tests_amount>();
    range_check_tests<pallas_field_type, 8, 65, random_tests_amount>();
    range_check_tests<pallas_field_type, 4, 63, random_tests_amount>();

    range_check_tests<vesta_field_type, 5, 254, random_tests_amount>();
    range_check_tests<vesta_field_type, 1, 22, random_tests_amount>();
    range_check_tests<vesta_field_type, 16, 129, random_tests_amount>();
}

BOOST_AUTO_TEST_CASE(blueprint_plonk_field_operations_test_to_fail) {
    using pallas_field_type = typename crypto3::algebra::curves::pallas::base_field_type;
    using vesta_field_type = typename crypto3::algebra::curves::vesta::base_field_type;

    range_check_tests_to_fail<pallas_field_type, 15, 30, random_tests_amount>();
    range_check_tests_to_fail<pallas_field_type, 10, 12, random_tests_amount>();
    range_check_tests_to_fail<pallas_field_type, 9, 128, random_tests_amount>();

    range_check_tests_to_fail<vesta_field_type, 11, 252, random_tests_amount>();
    range_check_tests_to_fail<vesta_field_type, 19, 220, random_tests_amount>();
    range_check_tests_to_fail<vesta_field_type, 5, 65, random_tests_amount>();
}

BOOST_AUTO_TEST_SUITE_END()