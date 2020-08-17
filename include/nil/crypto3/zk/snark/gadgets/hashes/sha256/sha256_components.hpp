//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//
// @file Declaration of interfaces for gadgets for the SHA256 message schedule and round function.
//---------------------------------------------------------------------------//

#ifndef SHA256_COMPONENTS_HPP_
#define SHA256_COMPONENTS_HPP_

#include <nil/crypto3/zk/snark/gadgets/basic_gadgets.hpp>
#include <nil/crypto3/zk/snark/gadgets/hashes/hash_io.hpp>
#include <nil/crypto3/zk/snark/gadgets/hashes/sha256/sha256_aux.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                const size_t SHA256_digest_size = 256;
                const size_t SHA256_block_size = 512;

                template<typename FieldType>
                pb_linear_combination_array<FieldType> SHA256_default_IV(protoboard<FieldType> &pb);

                template<typename FieldType>
                class sha256_message_schedule_gadget : public gadget<FieldType> {
                public:
                    std::vector<pb_variable_array<FieldType>> W_bits;
                    std::vector<std::shared_ptr<packing_gadget<FieldType>>> pack_W;

                    std::vector<pb_variable<FieldType>> sigma0;
                    std::vector<pb_variable<FieldType>> sigma1;
                    std::vector<std::shared_ptr<small_sigma_gadget<FieldType>>> compute_sigma0;
                    std::vector<std::shared_ptr<small_sigma_gadget<FieldType>>> compute_sigma1;
                    std::vector<pb_variable<FieldType>> unreduced_W;
                    std::vector<std::shared_ptr<lastbits_gadget<FieldType>>> mod_reduce_W;

                public:
                    pb_variable_array<FieldType> M;
                    pb_variable_array<FieldType> packed_W;
                    sha256_message_schedule_gadget(protoboard<FieldType> &pb,
                                                   const pb_variable_array<FieldType> &M,
                                                   const pb_variable_array<FieldType> &packed_W);
                    void generate_r1cs_constraints();
                    void generate_r1cs_witness();
                };

                template<typename FieldType>
                class sha256_round_function_gadget : public gadget<FieldType> {
                public:
                    pb_variable<FieldType> sigma0;
                    pb_variable<FieldType> sigma1;
                    std::shared_ptr<big_sigma_gadget<FieldType>> compute_sigma0;
                    std::shared_ptr<big_sigma_gadget<FieldType>> compute_sigma1;
                    pb_variable<FieldType> choice;
                    pb_variable<FieldType> majority;
                    std::shared_ptr<choice_gadget<FieldType>> compute_choice;
                    std::shared_ptr<majority_gadget<FieldType>> compute_majority;
                    pb_variable<FieldType> packed_d;
                    std::shared_ptr<packing_gadget<FieldType>> pack_d;
                    pb_variable<FieldType> packed_h;
                    std::shared_ptr<packing_gadget<FieldType>> pack_h;
                    pb_variable<FieldType> unreduced_new_a;
                    pb_variable<FieldType> unreduced_new_e;
                    std::shared_ptr<lastbits_gadget<FieldType>> mod_reduce_new_a;
                    std::shared_ptr<lastbits_gadget<FieldType>> mod_reduce_new_e;
                    pb_variable<FieldType> packed_new_a;
                    pb_variable<FieldType> packed_new_e;

                public:
                    pb_linear_combination_array<FieldType> a;
                    pb_linear_combination_array<FieldType> b;
                    pb_linear_combination_array<FieldType> c;
                    pb_linear_combination_array<FieldType> d;
                    pb_linear_combination_array<FieldType> e;
                    pb_linear_combination_array<FieldType> f;
                    pb_linear_combination_array<FieldType> g;
                    pb_linear_combination_array<FieldType> h;
                    pb_variable<FieldType> W;
                    long K;
                    pb_linear_combination_array<FieldType> new_a;
                    pb_linear_combination_array<FieldType> new_e;

                    sha256_round_function_gadget(protoboard<FieldType> &pb,
                                                 const pb_linear_combination_array<FieldType> &a,
                                                 const pb_linear_combination_array<FieldType> &b,
                                                 const pb_linear_combination_array<FieldType> &c,
                                                 const pb_linear_combination_array<FieldType> &d,
                                                 const pb_linear_combination_array<FieldType> &e,
                                                 const pb_linear_combination_array<FieldType> &f,
                                                 const pb_linear_combination_array<FieldType> &g,
                                                 const pb_linear_combination_array<FieldType> &h,
                                                 const pb_variable<FieldType> &W,
                                                 const long &K,
                                                 const pb_linear_combination_array<FieldType> &new_a,
                                                 const pb_linear_combination_array<FieldType> &new_e);

                    void generate_r1cs_constraints();
                    void generate_r1cs_witness();
                };

                const unsigned long SHA256_K[64] = {
                    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

                const unsigned long SHA256_H[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

                template<typename FieldType>
                pb_linear_combination_array<FieldType> SHA256_default_IV(protoboard<FieldType> &pb) {
                    pb_linear_combination_array<FieldType> result;
                    result.reserve(SHA256_digest_size);

                    for (size_t i = 0; i < SHA256_digest_size; ++i) {
                        int iv_val = (SHA256_H[i / 32] >> (31 - (i % 32))) & 1;

                        pb_linear_combination<FieldType> iv_element;
                        iv_element.assign(pb, iv_val * pb_variable<FieldType>(0));
                        iv_element.evaluate(pb);

                        result.emplace_back(iv_element);
                    }

                    return result;
                }

                template<typename FieldType>
                sha256_message_schedule_gadget<FieldType>::sha256_message_schedule_gadget(
                    protoboard<FieldType> &pb,
                    const pb_variable_array<FieldType> &M,
                    const pb_variable_array<FieldType> &packed_W) :
                    gadget<FieldType>(pb),
                    M(M), packed_W(packed_W) {
                    W_bits.resize(64);

                    pack_W.resize(16);
                    for (size_t i = 0; i < 16; ++i) {
                        W_bits[i] =
                            pb_variable_array<FieldType>(M.rbegin() + (15 - i) * 32, M.rbegin() + (16 - i) * 32);
                        pack_W[i].reset(new packing_gadget<FieldType>(pb, W_bits[i], packed_W[i]));
                    }

                    /* NB: some of those will be un-allocated */
                    sigma0.resize(64);
                    sigma1.resize(64);
                    compute_sigma0.resize(64);
                    compute_sigma1.resize(64);
                    unreduced_W.resize(64);
                    mod_reduce_W.resize(64);

                    for (size_t i = 16; i < 64; ++i) {
                        /* allocate result variables for sigma0/sigma1 invocations */
                        sigma0[i].allocate(pb);
                        sigma1[i].allocate(pb);

                        /* compute sigma0/sigma1 */
                        compute_sigma0[i].reset(
                            new small_sigma_gadget<FieldType>(pb, W_bits[i - 15], sigma0[i], 7, 18, 3));
                        compute_sigma1[i].reset(
                            new small_sigma_gadget<FieldType>(pb, W_bits[i - 2], sigma1[i], 17, 19, 10));

                        /* unreduced_W = sigma0(W_{i-15}) + sigma1(W_{i-2}) + W_{i-7} + W_{i-16} before modulo 2^32 */
                        unreduced_W[i].allocate(pb);

                        /* allocate the bit representation of packed_W[i] */
                        W_bits[i].allocate(pb, 32);

                        /* and finally reduce this into packed and bit representations */
                        mod_reduce_W[i].reset(
                            new lastbits_gadget<FieldType>(pb, unreduced_W[i], 32 + 2, packed_W[i], W_bits[i]));
                    }
                }

                template<typename FieldType>
                void sha256_message_schedule_gadget<FieldType>::generate_r1cs_constraints() {
                    for (size_t i = 0; i < 16; ++i) {
                        pack_W[i]->generate_r1cs_constraints(false);    // do not enforce bitness here; caller be aware.
                    }

                    for (size_t i = 16; i < 64; ++i) {
                        compute_sigma0[i]->generate_r1cs_constraints();
                        compute_sigma1[i]->generate_r1cs_constraints();

                        this->pb.add_r1cs_constraint(r1cs_constraint<FieldType>(
                            1, sigma0[i] + sigma1[i] + packed_W[i - 16] + packed_W[i - 7], unreduced_W[i]));

                        mod_reduce_W[i]->generate_r1cs_constraints();
                    }
                }

                template<typename FieldType>
                void sha256_message_schedule_gadget<FieldType>::generate_r1cs_witness() {
                    for (size_t i = 0; i < 16; ++i) {
                        pack_W[i]->generate_r1cs_witness_from_bits();
                    }

                    for (size_t i = 16; i < 64; ++i) {
                        compute_sigma0[i]->generate_r1cs_witness();
                        compute_sigma1[i]->generate_r1cs_witness();

                        this->pb.val(unreduced_W[i]) = this->pb.val(sigma0[i]) + this->pb.val(sigma1[i]) +
                                                       this->pb.val(packed_W[i - 16]) + this->pb.val(packed_W[i - 7]);
                        mod_reduce_W[i]->generate_r1cs_witness();
                    }
                }

                template<typename FieldType>
                sha256_round_function_gadget<FieldType>::sha256_round_function_gadget(
                    protoboard<FieldType> &pb,
                    const pb_linear_combination_array<FieldType> &a,
                    const pb_linear_combination_array<FieldType> &b,
                    const pb_linear_combination_array<FieldType> &c,
                    const pb_linear_combination_array<FieldType> &d,
                    const pb_linear_combination_array<FieldType> &e,
                    const pb_linear_combination_array<FieldType> &f,
                    const pb_linear_combination_array<FieldType> &g,
                    const pb_linear_combination_array<FieldType> &h,
                    const pb_variable<FieldType> &W,
                    const long &K,
                    const pb_linear_combination_array<FieldType> &new_a,
                    const pb_linear_combination_array<FieldType> &new_e) :
                    gadget<FieldType>(pb),
                    a(a), b(b), c(c), d(d), e(e), f(f), g(g), h(h), W(W), K(K), new_a(new_a), new_e(new_e) {
                    /* compute sigma0 and sigma1 */
                    sigma0.allocate(pb);
                    sigma1.allocate(pb);
                    compute_sigma0.reset(new big_sigma_gadget<FieldType>(pb, a, sigma0, 2, 13, 22));
                    compute_sigma1.reset(new big_sigma_gadget<FieldType>(pb, e, sigma1, 6, 11, 25));

                    /* compute choice */
                    choice.allocate(pb);
                    compute_choice.reset(new choice_gadget<FieldType>(pb, e, f, g, choice));

                    /* compute majority */
                    majority.allocate(pb);
                    compute_majority.reset(new majority_gadget<FieldType>(pb, a, b, c, majority));

                    /* pack d */
                    packed_d.allocate(pb);
                    pack_d.reset(new packing_gadget<FieldType>(pb, d, packed_d));

                    /* pack h */
                    packed_h.allocate(pb);
                    pack_h.reset(new packing_gadget<FieldType>(pb, h, packed_h));

                    /* compute the actual results for the round */
                    unreduced_new_a.allocate(pb);
                    unreduced_new_e.allocate(pb);

                    packed_new_a.allocate(pb);
                    packed_new_e.allocate(pb);

                    mod_reduce_new_a.reset(
                        new lastbits_gadget<FieldType>(pb, unreduced_new_a, 32 + 3, packed_new_a, new_a));
                    mod_reduce_new_e.reset(
                        new lastbits_gadget<FieldType>(pb, unreduced_new_e, 32 + 3, packed_new_e, new_e));
                }

                template<typename FieldType>
                void sha256_round_function_gadget<FieldType>::generate_r1cs_constraints() {
                    compute_sigma0->generate_r1cs_constraints();
                    compute_sigma1->generate_r1cs_constraints();

                    compute_choice->generate_r1cs_constraints();
                    compute_majority->generate_r1cs_constraints();

                    pack_d->generate_r1cs_constraints(false);
                    pack_h->generate_r1cs_constraints(false);

                    this->pb.add_r1cs_constraint(r1cs_constraint<FieldType>(
                        1, packed_h + sigma1 + choice + K + W + sigma0 + majority, unreduced_new_a));

                    this->pb.add_r1cs_constraint(
                        r1cs_constraint<FieldType>(1, packed_d + packed_h + sigma1 + choice + K + W, unreduced_new_e));

                    mod_reduce_new_a->generate_r1cs_constraints();
                    mod_reduce_new_e->generate_r1cs_constraints();
                }

                template<typename FieldType>
                void sha256_round_function_gadget<FieldType>::generate_r1cs_witness() {
                    compute_sigma0->generate_r1cs_witness();
                    compute_sigma1->generate_r1cs_witness();

                    compute_choice->generate_r1cs_witness();
                    compute_majority->generate_r1cs_witness();

                    pack_d->generate_r1cs_witness_from_bits();
                    pack_h->generate_r1cs_witness_from_bits();

                    this->pb.val(unreduced_new_a) = this->pb.val(packed_h) + this->pb.val(sigma1) +
                                                    this->pb.val(choice) + FieldType(K) + this->pb.val(W) +
                                                    this->pb.val(sigma0) + this->pb.val(majority);
                    this->pb.val(unreduced_new_e) = this->pb.val(packed_d) + this->pb.val(packed_h) +
                                                    this->pb.val(sigma1) + this->pb.val(choice) + FieldType(K) +
                                                    this->pb.val(W);

                    mod_reduce_new_a->generate_r1cs_witness();
                    mod_reduce_new_e->generate_r1cs_witness();
                }

            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // SHA256_COMPONENTS_HPP_
