//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
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

#ifndef CRYPTO3_USCS_PPZKSNARK_HPP
#define CRYPTO3_USCS_PPZKSNARK_HPP

#include <nil/crypto3/zk/snark/proof_systems/detail/ppzksnark/uscs_ppzksnark/types_policy.hpp>
#include <nil/crypto3/zk/snark/proof_systems/ppzksnark/policies/uscs_ppzksnark/generator.hpp>
#include <nil/crypto3/zk/snark/proof_systems/ppzksnark/policies/uscs_ppzksnark/prover.hpp>
#include <nil/crypto3/zk/snark/proof_systems/ppzksnark/policies/uscs_ppzksnark/verifier.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                template<typename CurveType,
                         typename Generator = policies::uscs_ppzksnark_generator<CurveType>,
                         typename Prover = policies::uscs_ppzksnark_prover<CurveType>,
                         typename Verifier = policies::uscs_ppzksnark_verifier_strong_input_consistency<CurveType>,
                         typename OnlineVerifier =
                             policies::uscs_ppzksnark_online_verifier_strong_input_consistency<CurveType>>
                class uscs_ppzksnark {
                    using types_policy = detail::uscs_ppzksnark_types_policy<CurveType>;

                public:
                    typedef typename types_policy::constraint_system constraint_system_type;
                    typedef typename types_policy::primary_input primary_input_type;
                    typedef typename types_policy::auxiliary_input auxiliary_input_type;

                    typedef typename types_policy::proving_key proving_key_type;
                    typedef typename types_policy::verification_key verification_key_type;
                    typedef typename types_policy::processed_verification_key processed_verification_key_type;

                    typedef typename types_policy::keypair keypair_type;
                    typedef typename types_policy::proof proof_type;

                    static inline keypair_type generator(const constraint_system_type &constraint_system) {
                        return Generator::process(constraint_system);
                    }

                    static inline proof_type prover(const proving_key_type &pk,
                                                    const primary_input_type &primary_input,
                                                    const auxiliary_input_type &auxiliary_input) {

                        return Prover::process(pk, primary_input, auxiliary_input);
                    }

                    static inline bool verifier(const verification_key_type &pvk,
                                                const primary_input_type &primary_input,
                                                const proof_type &proof) {
                        return Verifier::process(pvk, primary_input, proof);
                    }

                    static inline bool online_verifier(const processed_verification_key_type &pvk,
                                                       const primary_input_type &primary_input,
                                                       const proof_type &proof) {
                        return OnlineVerifier::process(pvk, primary_input, proof);
                    }
                };
            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_USCS_PPZKSNARK_HPP
