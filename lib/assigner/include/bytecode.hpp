//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_BYTECODE_HPP_
#define EVM1_ASSIGNER_INCLUDE_BYTECODE_HPP_

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/random/algebraic_engine.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        void process_bytecode_input(size_t original_code_size, const uint8_t* code,
                                    std::vector<assignment<crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>>> &assignments) {
            using value_type = typename BlueprintFieldType::value_type;
            using hash_type = nil::crypto3::hashes::keccak_1600<256>;

            std::cout << "MYINFO: <Enter dispatch function>" << std::endl;
            std::cout << "MYINFO: bytecode size: " << original_code_size << std::endl;
            std::cout << "MYINFO: bytecode: " << std::endl;

            std::vector<std::uint8_t> bytecode;
            for (size_t i = 0; i < original_code_size; i++) {
                const auto op = code[i];
                bytecode.push_back(op);
                std::cout << (uint)op << " ";
            }
            std::cout << std::endl;

            std::cout << "MYINFO: <get hashcode>:" << std::endl;
            std::string hash = nil::crypto3::hash<nil::crypto3::hashes::keccak_1600<256>>(bytecode.begin(), bytecode.end());
            std::string str_hi = hash.substr(0, hash.size()-32);
            std::string str_lo = hash.substr(hash.size()-32, 32);
            value_type hash_hi;
            value_type hash_lo;
            for( std::size_t j = 0; j < str_hi.size(); j++ ){hash_hi *=16; hash_hi += str_hi[j] >= '0' && str_hi[j] <= '9'? str_hi[j] - '0' : str_hi[j] - 'a' + 10;}
            for( std::size_t j = 0; j < str_lo.size(); j++ ){hash_lo *=16; hash_lo += str_lo[j] >= '0' && str_lo[j] <= '9'? str_lo[j] - '0' : str_lo[j] - 'a' + 10;}
            std::cout << std::hex <<  "Contract hash = " << hash << " h:" << hash_hi << " l:" << hash_lo << std::dec << std::endl;

            static constexpr uint32_t TAG = 0;
            static constexpr uint32_t INDEX = 1;
            static constexpr uint32_t VALUE = 2;
            static constexpr uint32_t IS_OPCODE = 3;
            static constexpr uint32_t PUSH_SIZE = 4;
            static constexpr uint32_t LENGTH_LEFT = 5;
            static constexpr uint32_t HASH_HI = 6;
            static constexpr uint32_t HASH_LO = 7;
            static constexpr uint32_t VALUE_RLC = 8;
            static constexpr uint32_t RLC_CHALLENGE = 9;

            value_type rlc_challenge = 15;
            uint32_t cur = 0;
            // no one another circuit uses witness column VALUE from 1th table
            uint32_t start_row_index = assignments[0].witness_column_size(VALUE);
            std::size_t prev_length = 0;
            value_type prev_vrlc = 0;
            value_type push_size = 0;
            for(size_t j = 0; j < original_code_size; j++, cur++){
                std::uint8_t byte = bytecode[j];
                assignments[0].witness(VALUE, start_row_index + cur) = bytecode[j];
                assignments[0].witness(HASH_HI, start_row_index + cur) = hash_hi;
                assignments[0].witness(HASH_LO, start_row_index + cur) = hash_lo;
                assignments[0].witness(RLC_CHALLENGE, start_row_index + cur) =  rlc_challenge;
                if( j == 0) {
                    // HEADER
                    assignments[0].witness(TAG, start_row_index + cur) =  0;
                    assignments[0].witness(INDEX, start_row_index + cur) = 0;
                    assignments[0].witness(IS_OPCODE, start_row_index + cur) = 0;
                    assignments[0].witness(PUSH_SIZE, start_row_index + cur) = 0;
                    prev_length = bytecode[j];
                    assignments[0].witness(LENGTH_LEFT, start_row_index + cur) = bytecode[j];
                    prev_vrlc = 0;
                    assignments[0].witness(VALUE_RLC, start_row_index + cur) = 0;
                    push_size = 0;
                } else {
                    // BYTE
                    assignments[0].witness(TAG, start_row_index + cur) = 1;
                    assignments[0].witness(INDEX, start_row_index + cur) =  j-1;
                    assignments[0].witness(LENGTH_LEFT, start_row_index + cur) = prev_length - 1;
                    prev_length = prev_length - 1;
                    if (push_size == 0) {
                        assignments[0].witness(IS_OPCODE, start_row_index + cur) = 1;
                        if(byte > 0x5f && byte < 0x80) {
                            push_size = byte - 0x5f;
                        }
                    } else {
                        assignments[0].witness(IS_OPCODE, start_row_index + cur) = 0;
                        push_size--;
                    }
                    assignments[0].witness(PUSH_SIZE, start_row_index + cur) = push_size;
                    assignments[0].witness(VALUE_RLC, start_row_index + cur) = prev_vrlc * rlc_challenge + byte;
                    prev_vrlc = prev_vrlc * rlc_challenge + byte;
                }
            }
        }

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_BYTECODE_HPP_
