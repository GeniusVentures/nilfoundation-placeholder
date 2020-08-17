//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//
// @file Declaration of public-parameter selector for the R1CS ppzkSNARK.
//---------------------------------------------------------------------------//

#ifndef R1CS_PPZKSNARK_PARAMS_HPP_
#define R1CS_PPZKSNARK_PARAMS_HPP_

#include <nil/crypto3/zk/snark/relations/constraint_satisfaction_problems/r1cs/r1cs.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                /**
                 * Below are various template aliases (used for convenience).
                 */

                template<typename ppT>
                using r1cs_ppzksnark_constraint_system = r1cs_constraint_system<algebra::Fr<ppT>>;

                template<typename ppT>
                using r1cs_ppzksnark_primary_input = r1cs_primary_input<algebra::Fr<ppT>>;

                template<typename ppT>
                using r1cs_ppzksnark_auxiliary_input = r1cs_auxiliary_input<algebra::Fr<ppT>>;

            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // R1CS_PPZKSNARK_PARAMS_HPP_
