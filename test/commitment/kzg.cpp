//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
// Copyright (c) 2022 Ekaterina Chukavina <kate@nil.foundation>
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

#define BOOST_TEST_MODULE kzg_test

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <nil/crypto3/algebra/random_element.hpp>
#include <nil/crypto3/algebra/algorithms/pair.hpp>
#include <nil/crypto3/algebra/curves/mnt4.hpp>
#include <nil/crypto3/algebra/pairing/mnt4.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/mnt4.hpp>
#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/pairing/bls12.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/bls12.hpp>
#include <nil/crypto3/algebra/curves/edwards.hpp>
#include <nil/crypto3/algebra/pairing/edwards.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/edwards.hpp>

#include <nil/crypto3/math/polynomial/polynomial.hpp>
#include <nil/crypto3/math/algorithms/unity_root.hpp>
#include <nil/crypto3/math/domains/evaluation_domain.hpp>
#include <nil/crypto3/math/algorithms/make_evaluation_domain.hpp>
#include <nil/crypto3/zk/commitments/polynomial/kzg.hpp>

using namespace nil::crypto3;
using namespace nil::crypto3::math;

BOOST_AUTO_TEST_SUITE(kzg_test_suite)

BOOST_AUTO_TEST_CASE(kzg_basic_test) {

    typedef algebra::curves::mnt4<298> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;
    typedef zk::commitments::kzg<curve_type> kzg_type;

    scalar_value_type alpha = 10;
    scalar_value_type i = 2;
    std::size_t n = 16;
    const polynomial<scalar_value_type> f = {-1, 1, 2, 3};

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);
    BOOST_CHECK(curve_type::template g1_type<>::value_type::one() == params.commitment_key[0]);
    BOOST_CHECK(10 * curve_type::template g1_type<>::value_type::one() == params.commitment_key[1]);
    BOOST_CHECK(100 * curve_type::template g1_type<>::value_type::one() == params.commitment_key[2]);
    BOOST_CHECK(1000 * curve_type::template g1_type<>::value_type::one() == params.commitment_key[3]);
    BOOST_CHECK(alpha * curve_type::template g2_type<>::value_type::one() == params.verification_key);

    auto commit = zk::algorithms::commit<kzg_type>(params, f);
    BOOST_CHECK(3209 * curve_type::template g1_type<>::value_type::one() == commit);

    auto eval = f.evaluate(i);
    auto proof = zk::algorithms::proof_eval<kzg_type>(params, f, i, eval);
    BOOST_CHECK(33 * scalar_value_type::one() == eval);
    BOOST_CHECK(397 * curve_type::template g1_type<>::value_type::one() == proof);

    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, commit, i, eval));
}

BOOST_AUTO_TEST_CASE(kzg_random_test) {

    typedef algebra::curves::mnt4<298> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;
    typedef zk::commitments::kzg<curve_type> kzg_type;

    scalar_value_type i = algebra::random_element<scalar_field_type>();
    scalar_value_type alpha = algebra::random_element<scalar_field_type>();
    std::size_t n = 298;
    const polynomial<scalar_value_type> f = {-1, 1, 2, 3, 5, -15};

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);
    auto commit = zk::algorithms::commit<kzg_type>(params, f);
    auto eval = f.evaluate(i);
    auto proof = zk::algorithms::proof_eval<kzg_type>(params, f, i, eval);

    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, commit, i, eval));
}

BOOST_AUTO_TEST_CASE(kzg_false_test) {

    typedef algebra::curves::mnt4<298> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;
    typedef zk::commitments::kzg<curve_type> kzg_type;

    scalar_value_type alpha = 10;
    scalar_value_type i = 2;
    std::size_t n = 16;
    const polynomial<scalar_value_type> f = {100, 1, 2, 3};

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);

    auto commit = zk::algorithms::commit<kzg_type>(params, f);

    auto eval = f.evaluate(i);
    auto proof = zk::algorithms::proof_eval<kzg_type>(params, f, i, eval);

    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, commit, i, eval));

    // wrong params
    auto ck2 = params.commitment_key;
    ck2[0] = ck2[0] * 2;
    auto params2 = kzg_type::params_type(ck2, params.verification_key * 2);
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params2, proof, commit, i, eval));

    // wrong commit
    auto commit2 = commit * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, commit2, i, eval));

    // wrong i
    auto i2 = i * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, commit, i2, eval));

    // wrong eval
    auto eval2 = eval * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, commit, i, eval2));

    // wrong proof
    {
        // wrong params
        typename kzg_type::proof_type proof2;
        bool exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params2, f, i, eval);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, commit, i, eval), "wrong params");
        }

        // wrong i
        exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params, f, i2, eval);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, commit, i, eval), "wrong i");
        }

        // wrong eval
        exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params, f, i, eval2);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, commit, i, eval), "wrong eval");
        }
    }
    auto proof2 = proof * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof2, commit, i, eval));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(batched_kzg_test_suite)

BOOST_AUTO_TEST_CASE(kzg_batched_accumulate_test) {

    typedef algebra::curves::mnt4<298> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;
    
    typedef zk::commitments::batched_kzg_params<2> batched_kzg_params_type;
    typedef zk::commitments::batched_kzg<curve_type, batched_kzg_params_type> kzg_type;

    {
        const std::vector<polynomial<scalar_value_type>> polynomials = {{
            {1, 2, 3, 4}, // 1 + 2x + 2x^2 + 3x^3
        }};
        const scalar_value_type beta = 29;

        const polynomial<scalar_value_type> expect_result = {1, 2, 3, 4};

        BOOST_CHECK(expect_result == zk::algorithms::accumulate<kzg_type>(polynomials, beta));
    }

    {
        const std::vector<polynomial<scalar_value_type>> polynomials = {{
            {1, 2, 3, 4}, // 1 + 2x + 2x^2 + 3x^3
            {5, 6, 7},
            {8, 9, 10, 11, 12},
        }};
        const scalar_value_type beta = 29;

        const polynomial<scalar_value_type> expect_result = {
            1 + beta * 5 + beta * beta * 8,
            2 + beta * 6 + beta * beta * 9,
            3 + beta * 7 + beta * beta * 10,
            4 + beta * 0 + beta * beta * 11,
            0 + beta * 0 + beta * beta * 12
        };

        BOOST_CHECK(expect_result == zk::algorithms::accumulate<kzg_type>(polynomials, beta));
    }

    {
        const std::vector<polynomial<scalar_value_type>> f_set{
            {1, 2, 3, 4, 5, 6, 7, 8},
            {11, 12, 0, 14, 15, 16, 17},
        };
        const scalar_value_type beta = 29;

        const polynomial<scalar_value_type> expect{
            1 + beta * 11,
            2 + beta * 12,
            3 + beta * 0,
            4 + beta * 14,
            5 + beta * 15,
            6 + beta * 16,
            7 + beta * 17,
            8 + beta * 0};

        const polynomial<scalar_value_type> actual =
            zk::algorithms::accumulate<kzg_type>(f_set, beta);

        BOOST_CHECK(expect == actual);
    }
}

BOOST_AUTO_TEST_CASE(kzg_batched_basic_test) {

    typedef algebra::curves::bls12<381> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;

    typedef zk::commitments::batched_kzg_params<2> batched_kzg_params_type;
    typedef zk::commitments::batched_kzg<curve_type, batched_kzg_params_type> kzg_type;

    scalar_value_type alpha = 7;
    std::size_t n = 8;
    const std::vector<polynomial<scalar_value_type>> fs{{
        {{1, 2, 3, 4, 5, 6, 7, 8}},
        {{11, 12, 13, 14, 15, 16, 17, 18}},
        {{21, 22, 23, 24, 25, 26, 27, 28}},
        {{31, 32, 33, 34, 35, 36, 37, 38}},
    }};
    const std::vector<polynomial<scalar_value_type>> gs{{
        {{71, 72, 73, 74, 75, 76, 77, 78}},
        {{81, 82, 83, 84, 85, 86, 87, 88}},
        {{91, 92, 93, 94, 95, 96, 97, 98}},
    }};
    typename kzg_type::batch_of_batches_of_polynomials_type polys = {fs, gs};
    
    std::vector<scalar_value_type> zs = {123, 456};
    auto evals = zk::algorithms::evaluate_polynomials<kzg_type>(polys, zs);

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);
    
    std::vector<scalar_value_type> gammas = {54321, 98760};

    auto proof = zk::algorithms::proof_eval<kzg_type>(params, polys, evals, zs, gammas);

    for (size_t j = 0; j < proof.size(); ++j) {
        scalar_value_type h0_x = scalar_value_type::zero();
        for (size_t i = 0; i < polys[j].size(); ++i) {
            const polynomial<scalar_value_type> &f_i = polys[j][i];
            const scalar_value_type f_x_minus_f_z0 = f_i.evaluate(alpha) - f_i.evaluate(zs[j]);
            const scalar_value_type gamma_power = gammas[j].pow(i);
            h0_x += gamma_power * f_x_minus_f_z0 * ((alpha - zs[j]).inversed());
        }
        BOOST_CHECK(h0_x * curve_type::template g1_type<>::value_type::one() == proof[j]);
    }

    scalar_value_type r = 23546;
    auto c0 = zk::algorithms::commit<kzg_type>(params, fs);
    auto c1 = zk::algorithms::commit<kzg_type>(params, gs);
    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, evals, {c0, c1}, zs, gammas, r));
}

BOOST_AUTO_TEST_CASE(kzg_batched_random_test) {

    typedef algebra::curves::bls12<381> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;

    typedef zk::commitments::batched_kzg_params<2> batched_kzg_params_type;
    typedef zk::commitments::batched_kzg<curve_type, batched_kzg_params_type> kzg_type;

    std::size_t n = 298;
    scalar_value_type alpha = algebra::random_element<scalar_field_type>();
    const std::vector<polynomial<scalar_value_type>> f0{{
        {{1, 2, 3, 4, 5, 6, 7, 8}},
        {{11, 12, 13, 14, 15, 16, 17}},
        {{21, 22, 23, 24, 25, 26, 27, 28}},
        {{31, 32, 33, 34, 35, 36, 37, 38, 39}},
    }};
    const std::vector<polynomial<scalar_value_type>> f1{{
        {{71, 72}},
        {{81, 82, 83, 85, 86, 87, 88}},
        {{91, 92, 93, 94, 95, 96, 97, 98, 99, 100}},
    }};
    const std::vector<polynomial<scalar_value_type>> f2{{
        {{73, 74, 25}},
        {{87}},
        {{91, 92, 93, 94, 95, 96, 97, 100, 1, 2, 3}},
    }};
    const kzg_type::batch_of_batches_of_polynomials_type polys = {f0, f1, f2};
    std::size_t num_polys = polys.size();

    std::vector<scalar_value_type> zs;
    for (std::size_t i = 0; i < num_polys; ++i) {
        zs.push_back(algebra::random_element<scalar_field_type>());
    }
    auto evals = zk::algorithms::evaluate_polynomials<kzg_type>(polys, zs);

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);
    
    std::vector<scalar_value_type> gammas;
    for (std::size_t i = 0; i < num_polys; ++i) {
        gammas.push_back(algebra::random_element<scalar_field_type>());
    }

    auto proof = zk::algorithms::proof_eval<kzg_type>(params, polys, evals, zs, gammas);

    for (std::size_t j = 0; j < proof.size(); ++j) {
        scalar_value_type h0_x = scalar_value_type::zero();
        for (std::size_t i = 0; i < polys[j].size(); ++i) {
            const polynomial<scalar_value_type> &f_i = polys[j][i];
            const scalar_value_type f_x_minus_f_z0 = f_i.evaluate(alpha) - f_i.evaluate(zs[j]);
            const scalar_value_type gamma_power = gammas[j].pow(i);
            h0_x += gamma_power * f_x_minus_f_z0 * ((alpha - zs[j]).inversed());
        }
        BOOST_CHECK(h0_x * curve_type::template g1_type<>::value_type::one() == proof[j]);
    }

    scalar_value_type r = algebra::random_element<scalar_field_type>();
    std::vector<std::vector<kzg_type::commitment_type>> cs;
    for (std::size_t j = 0; j < num_polys; ++j) {
        cs.push_back(zk::algorithms::commit<kzg_type>(params, polys[j]));
    }
    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, evals, cs, zs, gammas, r));
}

BOOST_AUTO_TEST_CASE(kzg_batched_false_test) {

    typedef algebra::curves::bls12<381> curve_type;
    typedef typename curve_type::base_field_type::value_type base_value_type;
    typedef typename curve_type::base_field_type base_field_type;
    typedef typename curve_type::scalar_field_type scalar_field_type;
    typedef typename curve_type::scalar_field_type::value_type scalar_value_type;

    typedef zk::commitments::batched_kzg_params<2> batched_kzg_params_type;
    typedef zk::commitments::batched_kzg<curve_type, batched_kzg_params_type> kzg_type;

    scalar_value_type alpha = 7;
    std::size_t n = 298;
    const std::vector<polynomial<scalar_value_type>> fs{{
        {{1, 2, 3, 4, 5, 6, 7, 8}},
        {{11, 12, 13, 14, 15, 16, 17, 18}},
        {{21, 22, 23, 24, 25, 26, 27, 28}},
        {{31, 32, 33, 34, 35, 36, 37, 38}},
    }};
    const std::vector<polynomial<scalar_value_type>> gs{{
        {{71, 72, 73, 74, 75, 76, 77, 78}},
        {{81, 82, 83, 84, 85, 86, 87, 88}},
        {{91, 92, 93, 94, 95, 96, 97, 98}},
    }};
    const std::vector<polynomial<scalar_value_type>> hs{{
        {{71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81}},
    }};
    typename kzg_type::batch_of_batches_of_polynomials_type polys = {fs, gs, hs};
    std::size_t num_polys = polys.size();
    
    std::vector<scalar_value_type> zs = {123, 456, 789};
    auto evals = zk::algorithms::evaluate_polynomials<kzg_type>(polys, zs);

    auto params = zk::algorithms::setup<kzg_type>(n, alpha);
    
    std::vector<scalar_value_type> gammas = {54321, 98760, 12345};

    auto proof = zk::algorithms::proof_eval<kzg_type>(params, polys, evals, zs, gammas);

    scalar_value_type r = 23546;
    std::vector<std::vector<kzg_type::commitment_type>> cs;
    for (std::size_t j = 0; j < num_polys; ++j) {
        cs.push_back(zk::algorithms::commit<kzg_type>(params, polys[j]));
    }
    BOOST_CHECK(zk::algorithms::verify_eval<kzg_type>(params, proof, evals, cs, zs, gammas, r));

    // wrong verification key
    auto ck2 = params.commitment_key;
    ck2[0] = ck2[0] * 2;
    auto params2 = kzg_type::params_type(ck2, params.verification_key * 2);
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params2, proof, evals, cs, zs, gammas, r));

    // wrong evals
    auto evals2 = evals;
    evals2[evals.size() / 2][0] = evals2[evals.size() / 2][0] * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, evals2, cs, zs, gammas, r));

    // wrong commitments
    auto cs2 = cs;
    cs2[0].back() = cs2[0].back() * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, evals, cs2, zs, gammas, r));

    // wrong zs
    auto zs2 = zs;
    zs2[zs2.size() / 2] = zs2[zs2.size() / 2] * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, evals, cs, zs2, gammas, r));

    // wrong gammas
    auto gammas2 = gammas;
    gammas2[gammas2.size() / 2] = gammas2[gammas2.size() / 2] * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof, evals, cs, zs, gammas2, r));

    // wrong proof
    {
        // wrong params
        typename kzg_type::batched_proof_type proof2;
        bool exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params2, polys, evals, zs, gammas);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, evals, cs, zs, gammas, r), "wrong params");
        }
        
        // wrong evals
        exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params, polys, evals2, zs, gammas);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, evals, cs, zs, gammas, r), "wrong evals");
        }

        // wrong zs
        exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params, polys, evals, zs2, gammas);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, evals, cs, zs, gammas, r), "wrong zs");
        }

        // wrong gammas
        exception = false;
        try {proof2 = zk::algorithms::proof_eval<kzg_type>(params, polys, evals, zs, gammas2);}
        catch (std::runtime_error& e) {exception = true;}
        if (!exception) {
            BOOST_CHECK(proof2 != proof);
            BOOST_CHECK_MESSAGE(!zk::algorithms::verify_eval<kzg_type>(params, proof2, evals, cs, zs, gammas, r), "wrong gammas");
        }
    }
    auto proof2 = proof;
    proof2.back() = proof2.back() * 2;
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params, proof2, evals, cs, zs, gammas, r));

    // wrong combination of all
    BOOST_CHECK(!zk::algorithms::verify_eval<kzg_type>(params2, proof2, evals2, cs2, zs2, gammas2, r));
}

BOOST_AUTO_TEST_SUITE_END()