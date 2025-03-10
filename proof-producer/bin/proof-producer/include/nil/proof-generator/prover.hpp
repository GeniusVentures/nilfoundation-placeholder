//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022-2023 Ilia Shirobokov <i.shirobokov@nil.foundation>
// Copyright (c) 2024 Iosif (x-mass) <x-mass@nil.foundation>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//

#ifndef PROOF_GENERATOR_ASSIGNER_PROOF_HPP
#define PROOF_GENERATOR_ASSIGNER_PROOF_HPP

#include <fstream>
#include <functional>
#include <ostream>
#include <random>
#include <sstream>
#include <optional>

#include <boost/log/trivial.hpp>

#ifndef TIME_LOG_ENABLED
#define TIME_LOG_ENABLED
#include <nil/crypto3/bench/scoped_profiler.hpp>
#endif

#include <nil/marshalling/endianness.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/status_type.hpp>

#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/eval_storage.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/lpc.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/common_data.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/preprocessed_public_data.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/proof.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/assignment_table.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>

#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/profiling.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>
#include <nil/crypto3/zk/transcript/fiat_shamir.hpp>

#include <nil/blueprint/transpiler/recursive_verifier_generator.hpp>
#include <nil/blueprint/transpiler/lpc_evm_verifier_gen.hpp>

#include <nil/proof-generator/preset/preset.hpp>
#include <nil/proof-generator/assigner/assigner.hpp>
#include <nil/proof-generator/arithmetization_params.hpp>
#include <nil/proof-generator/output_artifacts/assignment_table_writer.hpp>
#include <nil/proof-generator/output_artifacts/circuit_writer.hpp>
#include <nil/proof-generator/output_artifacts/output_artifacts.hpp>
#include <nil/proof-generator/file_operations.hpp>

#include <nil/blueprint/blueprint/plonk/circuit.hpp>

namespace nil {
    namespace proof_generator {
        namespace detail {
            template<typename MarshallingType>
            std::optional<MarshallingType> decode_marshalling_from_file(
                const boost::filesystem::path& path,
                bool hex = false
            ) {
                const auto v = hex ? read_hex_file_to_vector(path.c_str()) : read_file_to_vector(path.c_str());
                if (!v.has_value()) {
                    return std::nullopt;
                }

                MarshallingType marshalled_data;
                auto read_iter = v->begin();
                auto status = marshalled_data.read(read_iter, v->size());
                if (status != nil::crypto3::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "When reading a Marshalled structure from file "
                        << path << ", decoding step failed.";
                    return std::nullopt;
                }
                return marshalled_data;
            }

            template<typename MarshallingType>
            bool encode_marshalling_to_file(
                const boost::filesystem::path& path,
                const MarshallingType& data_for_marshalling,
                bool hex = false
            ) {
                std::vector<std::uint8_t> v;
                v.resize(data_for_marshalling.length(), 0x00);
                auto write_iter = v.begin();
                nil::crypto3::marshalling::status_type status = data_for_marshalling.write(write_iter, v.size());
                if (status != nil::crypto3::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "Marshalled structure encoding failed";
                    return false;
                }

                return hex ? write_vector_to_hex_file(v, path.c_str()) : write_vector_to_file(v, path.c_str());
            }

            enum class ProverStage {
                ALL = 0,
                PRESET = 1,
                ASSIGNMENT = 2,
                PREPROCESS = 3,
                PROVE = 4,
                VERIFY = 5,
                GENERATE_AGGREGATED_CHALLENGE = 6,
                GENERATE_PARTIAL_PROOF = 7,
                FAST_GENERATE_PARTIAL_PROOF = 8,
                COMPUTE_COMBINED_Q = 9,
                GENERATE_AGGREGATED_FRI_PROOF = 10,
                GENERATE_CONSISTENCY_CHECKS_PROOF = 11,
                MERGE_PROOFS = 12
            };

            ProverStage prover_stage_from_string(const std::string& stage) {
                static std::unordered_map<std::string, ProverStage> stage_map = {
                    {"all", ProverStage::ALL},
                    {"preset", ProverStage::PRESET},
                    {"fill-assignment", ProverStage::ASSIGNMENT},
                    {"preprocess", ProverStage::PREPROCESS},
                    {"prove", ProverStage::PROVE},
                    {"verify", ProverStage::VERIFY},
                    {"generate-aggregated-challenge", ProverStage::GENERATE_AGGREGATED_CHALLENGE},
                    {"generate-partial-proof", ProverStage::GENERATE_PARTIAL_PROOF},
                    {"fast-generate-partial-proof", ProverStage::FAST_GENERATE_PARTIAL_PROOF},
                    {"compute-combined-Q", ProverStage::COMPUTE_COMBINED_Q},
                    {"merge-proofs", ProverStage::MERGE_PROOFS},
                    {"aggregated-FRI", ProverStage::GENERATE_AGGREGATED_FRI_PROOF},
                    {"consistency-checks", ProverStage::GENERATE_CONSISTENCY_CHECKS_PROOF}
                };
                auto it = stage_map.find(stage);
                if (it == stage_map.end()) {
                    throw std::invalid_argument("Invalid stage: " + stage);
                }
                return it->second;
            }

        } // namespace detail


        template<typename CurveType, typename HashType>
        class Prover {
        public:
            using BlueprintField = typename CurveType::base_field_type;
            using LpcParams = nil::crypto3::zk::commitments::list_polynomial_commitment_params<HashType, HashType, 2>;
            using Lpc = nil::crypto3::zk::commitments::list_polynomial_commitment<BlueprintField, LpcParams>;
            using LpcScheme = typename nil::crypto3::zk::commitments::lpc_commitment_scheme<Lpc>;
            using polynomial_type = typename LpcScheme::polynomial_type;
            using CircuitParams = nil::crypto3::zk::snark::placeholder_circuit_params<BlueprintField>;
            using PlaceholderParams = nil::crypto3::zk::snark::placeholder_params<CircuitParams, LpcScheme>;
            using Proof = nil::crypto3::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>;
            using PublicPreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using CommonData = typename PublicPreprocessedData::common_data_type;
            using PrivatePreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using ConstraintSystem = nil::blueprint::circuit<nil::crypto3::zk::snark::plonk_constraint_system<BlueprintField>>;
            using TableDescription = nil::crypto3::zk::snark::plonk_table_description<BlueprintField>;
            using Endianness = nil::crypto3::marshalling::option::big_endian;
            using FriType = typename Lpc::fri_type;
            using FriParams = typename FriType::params_type;
            using Column = nil::crypto3::zk::snark::plonk_column<BlueprintField>;
            using AssignmentTable = nil::crypto3::zk::snark::plonk_assignment_table<BlueprintField>;
            using TTypeBase = nil::crypto3::marshalling::field_type<Endianness>;

            using TableMarshalling = nil::crypto3::marshalling::types::plonk_assignment_table<TTypeBase, AssignmentTable>;

            Prover(
                std::size_t lambda,
                std::size_t expand_factor,
                std::size_t max_q_chunks,
                std::size_t grind,
                std::string circuit_name
            ) : expand_factor_(expand_factor),
                max_quotient_chunks_(max_q_chunks),
                lambda_(lambda),
                grind_(grind),
                circuit_name_(circuit_name){
            }

            bool print_evm_verifier(
                boost::filesystem::path output_folder
            ){
                if( output_folder.empty() ) return true;
                BOOST_LOG_TRIVIAL(info) << "Print evm verifier";
                nil::blueprint::lpc_evm_verifier_printer<PlaceholderParams> evm_verifier_printer(
                    *constraint_system_,
                    public_preprocessed_data_->common_data,
                    output_folder.string()
                );
                evm_verifier_printer.print();
                return true;
            }

            bool print_public_input_for_evm(
                boost::filesystem::path output_folder
            ){
                if( output_folder.empty() ) return true;
                BOOST_LOG_TRIVIAL(info) << "Print public input for EVM";
                std::ofstream pi_stream;
                pi_stream.open(output_folder.string() + "/public_input.inp");
                if( !pi_stream.is_open() )  return false;

                // Does not support public input columns.
                if( table_description_->public_input_columns != 0 ) {
                    std::size_t max_non_zero = 0;
                    const auto&public_input = assignment_table_->public_input(0);
                    for (std::size_t i = 0; i < public_input.size(); i++) {
                        if (public_input[i] != 0u) {
                            max_non_zero = i + 1;
                        }
                    }
                    for (std::size_t i = 0; i < std::min(public_input.size(), max_non_zero); i++) {
                        pi_stream << public_input[i] << "\n";
                    }
                } // else empty file is generated
                pi_stream.close();
                return true;
            }

            // The caller must call the preprocessor or load the preprocessed data before calling this function.
            bool generate_to_file(
                    boost::filesystem::path proof_file_,
                    boost::filesystem::path json_file_,
                    bool skip_verification) {
                if (!can_write_to_file(proof_file_.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't write to file " << proof_file_;
                    return false;
                }

                BOOST_ASSERT(public_preprocessed_data_);
                BOOST_ASSERT(private_preprocessed_data_);
                BOOST_ASSERT(table_description_);
                BOOST_ASSERT(constraint_system_);
                BOOST_ASSERT(lpc_scheme_);

                BOOST_LOG_TRIVIAL(info) << "Generating proof...";
                TIME_LOG_START("Generation Proof")
                nil::crypto3::zk::snark::placeholder_prover<BlueprintField, PlaceholderParams> prover(
                    *public_preprocessed_data_,
                    *private_preprocessed_data_,
                    *table_description_,
                    *constraint_system_,
                    std::move(*lpc_scheme_)
                );
                auto proof = prover.process();

                BOOST_LOG_TRIVIAL(info) << "Proof generated";
                TIME_LOG_END("Generation Proof")

                create_lpc_scheme(); // reset to default scheme to do the verification
                bool verify_ok{};
                if (skip_verification) {
                    BOOST_LOG_TRIVIAL(info) << "Skipping proof verification";
                    verify_ok = true;
                } else {
                    TIME_LOG_SCOPE("Verification Proof")
                    verify_ok = verify(proof);
                }
                lpc_scheme_.emplace(std::move(prover.move_commitment_scheme())); // get back the commitment scheme used in prover
                BOOST_LOG_TRIVIAL(info) << "Proof verified";

                if (!verify_ok) {
                    BOOST_LOG_TRIVIAL(error) << "Proof verification failed";
                    return false;
                }

                BOOST_LOG_TRIVIAL(info) << "Writing proof to " << proof_file_;
                auto filled_placeholder_proof =
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, Proof>(proof, lpc_scheme_->get_fri_params());
                bool res = detail::encode_marshalling_to_file(
                    proof_file_,
                    filled_placeholder_proof,
                    true
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof written.";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Failed to write proof to file.";
                }

                BOOST_LOG_TRIVIAL(info) << "Writing json proof to " << json_file_;
                auto output_file = open_file<std::ofstream>(json_file_.string(), std::ios_base::out);
                if (!output_file)
                    return res;

                (*output_file) << nil::blueprint::recursive_verifier_generator<
                                      PlaceholderParams,
                                      nil::crypto3::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>,
                                      typename nil::crypto3::zk::snark::placeholder_public_preprocessor<
                                          BlueprintField,
                                          PlaceholderParams>::preprocessed_data_type::common_data_type>(
                                      *table_description_
                ).generate_input(*public_inputs_, proof, constraint_system_->public_input_sizes());
                output_file->close();
                return res;
            }

            // The caller must call the preprocessor or load the preprocessed data before calling this function.
            bool generate_partial_proof_to_file(
                    boost::filesystem::path proof_file_,
                    std::optional<boost::filesystem::path> challenge_file_,
                    std::optional<boost::filesystem::path> theta_power_file) {
                if (!can_write_to_file(proof_file_.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't write to file " << proof_file_;
                    return false;
                }

                BOOST_ASSERT(public_preprocessed_data_);
                BOOST_ASSERT(private_preprocessed_data_);
                BOOST_ASSERT(table_description_);
                BOOST_ASSERT(constraint_system_);
                BOOST_ASSERT(lpc_scheme_);

                BOOST_LOG_TRIVIAL(info) << "Generating proof...";
                TIME_LOG_START("Generation Proof")
                auto prover = nil::crypto3::zk::snark::placeholder_prover<BlueprintField, PlaceholderParams>(
                        *public_preprocessed_data_,
                        *private_preprocessed_data_,
                        *table_description_,
                        *constraint_system_,
                        std::move(*lpc_scheme_),
                        true);
                Proof proof = prover.process();

                BOOST_LOG_TRIVIAL(info) << "Proof generated";
                TIME_LOG_END("Generation Proof")

                lpc_scheme_.emplace(prover.move_commitment_scheme()); // get back the commitment scheme used in prover

                BOOST_LOG_TRIVIAL(info) << "Writing proof to " << proof_file_;
                auto filled_placeholder_proof =
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, Proof>(proof, lpc_scheme_->get_fri_params());
                bool res = detail::encode_marshalling_to_file(
                    proof_file_,
                    filled_placeholder_proof,
                    true
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof written.";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Failed to write proof to file.";
                }

                if (!challenge_file_) {
                    BOOST_LOG_TRIVIAL(error) << "Challenge output file is not set.";
                    return false;
                }
                if (!theta_power_file) {
                    BOOST_LOG_TRIVIAL(error) << "Theta power file is not set.";
                    return false;
                }
                BOOST_LOG_TRIVIAL(info) << "Writing challenge";
                using challenge_marshalling_type =
                    nil::crypto3::marshalling::types::field_element<
                    TTypeBase, typename BlueprintField::value_type>;

                challenge_marshalling_type marshalled_challenge(proof.eval_proof.challenge);

                res = detail::encode_marshalling_to_file<challenge_marshalling_type>(
                            *challenge_file_, marshalled_challenge);
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Challenge written.";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Failed to write challenge to file.";
                }

                lpc_scheme_->state_commited(crypto3::zk::snark::FIXED_VALUES_BATCH);
                lpc_scheme_->state_commited(crypto3::zk::snark::VARIABLE_VALUES_BATCH);
                lpc_scheme_->state_commited(crypto3::zk::snark::PERMUTATION_BATCH);
                lpc_scheme_->state_commited(crypto3::zk::snark::QUOTIENT_BATCH);
                lpc_scheme_->state_commited(crypto3::zk::snark::LOOKUP_BATCH);
                lpc_scheme_->mark_batch_as_fixed(crypto3::zk::snark::FIXED_VALUES_BATCH);

                lpc_scheme_->set_fixed_polys_values(common_data_.has_value() ? common_data_->commitment_scheme_data :
                                                                                    public_preprocessed_data_->common_data.commitment_scheme_data);

                std::size_t theta_power = lpc_scheme_->compute_theta_power_for_combined_Q();

                auto output_file = open_file<std::ofstream>(theta_power_file->string(), std::ios_base::out);
                (*output_file) << theta_power << std::endl;
                output_file->close();

                return res;
            }

            bool verify_from_file(boost::filesystem::path proof_file_) {
                create_lpc_scheme();

                using ProofMarshalling = nil::crypto3::marshalling::types::
                    placeholder_proof<nil::crypto3::marshalling::field_type<Endianness>, Proof>;

                BOOST_LOG_TRIVIAL(info) << "Reading proof from file";
                auto marshalled_proof = detail::decode_marshalling_from_file<ProofMarshalling>(proof_file_, true);
                if (!marshalled_proof) {
                    return false;
                }
                bool res = verify(nil::crypto3::marshalling::types::make_placeholder_proof<Endianness, Proof>(
                    *marshalled_proof));
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof verification passed.";
                }
                return res;
            }

            bool save_preprocessed_common_data_to_file(boost::filesystem::path preprocessed_common_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Writing preprocessed common data to " << preprocessed_common_data_file;
                auto marshalled_common_data =
                    nil::crypto3::marshalling::types::fill_placeholder_common_data<Endianness, CommonData>(
                        public_preprocessed_data_->common_data
                    );
                bool res = detail::encode_marshalling_to_file(
                    preprocessed_common_data_file,
                    marshalled_common_data
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Preprocessed common data written.";
                }
                return res;
            }

            bool read_preprocessed_common_data_from_file(boost::filesystem::path preprocessed_common_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Read preprocessed common data from " << preprocessed_common_data_file;

                using CommonDataMarshalling = nil::crypto3::marshalling::types::placeholder_common_data<TTypeBase, CommonData>;

                auto marshalled_value = detail::decode_marshalling_from_file<CommonDataMarshalling>(
                    preprocessed_common_data_file);

                if (!marshalled_value) {
                    return false;
                }
                common_data_.emplace(nil::crypto3::marshalling::types::make_placeholder_common_data<Endianness, CommonData>(
                    *marshalled_value));

                return true;
            }

            // This includes not only the common data, but also merkle trees, polynomials, etc, everything that a
            // public preprocessor generates.
            bool save_public_preprocessed_data_to_file(boost::filesystem::path preprocessed_data_file) {
                using namespace nil::crypto3::marshalling::types;

                BOOST_LOG_TRIVIAL(info) << "Writing all preprocessed public data to " <<
                    preprocessed_data_file;

                auto marshalled_preprocessed_public_data =
                    fill_placeholder_preprocessed_public_data<Endianness, PublicPreprocessedData>(
                        *public_preprocessed_data_
                    );
                bool res = detail::encode_marshalling_to_file(
                    preprocessed_data_file,
                    marshalled_preprocessed_public_data
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Preprocessed public data written.";
                }
                return res;
            }

            bool read_public_preprocessed_data_from_file(boost::filesystem::path preprocessed_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Read preprocessed data from " << preprocessed_data_file;

                using namespace nil::crypto3::marshalling::types;

                using PublicPreprocessedDataMarshalling =
                    placeholder_preprocessed_public_data<TTypeBase, PublicPreprocessedData>;

                auto marshalled_value = detail::decode_marshalling_from_file<PublicPreprocessedDataMarshalling>(
                    preprocessed_data_file);
                if (!marshalled_value) {
                    return false;
                }
                public_preprocessed_data_.emplace(
                    make_placeholder_preprocessed_public_data<Endianness, PublicPreprocessedData>(*marshalled_value)
                );
                return true;
            }

            bool save_commitment_state_to_file(boost::filesystem::path commitment_scheme_state_file) {
                using namespace nil::crypto3::marshalling::types;

                BOOST_LOG_TRIVIAL(info) << "Writing commitment_state to " <<
                    commitment_scheme_state_file;

                auto marshalled_lpc_state = fill_commitment_scheme<Endianness, LpcScheme>(
                    *lpc_scheme_);
                bool res = detail::encode_marshalling_to_file(
                    commitment_scheme_state_file,
                    marshalled_lpc_state
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Commitment scheme written.";
                }
                return res;
            }

            bool read_commitment_scheme_from_file(boost::filesystem::path commitment_scheme_state_file) {
                BOOST_LOG_TRIVIAL(info) << "Read commitment scheme from " << commitment_scheme_state_file;

                using namespace nil::crypto3::marshalling::types;

                using CommitmentStateMarshalling = typename commitment_scheme_state<TTypeBase, LpcScheme>::type;

                auto marshalled_value = detail::decode_marshalling_from_file<CommitmentStateMarshalling>(
                    commitment_scheme_state_file);

                if (!marshalled_value) {
                    return false;
                }

                auto commitment_scheme = make_commitment_scheme<Endianness, LpcScheme>(*marshalled_value);
                if (!commitment_scheme) {
                    BOOST_LOG_TRIVIAL(error) << "Error decoding commitment scheme";
                    return false;
                }

                lpc_scheme_.emplace(std::move(commitment_scheme.value()));
                return true;
            }

            bool verify(const Proof& proof) {
                BOOST_LOG_TRIVIAL(info) << "Verifying proof...";
                bool verification_result = nil::crypto3::zk::snark::placeholder_verifier<BlueprintField, PlaceholderParams>::process(
                        public_preprocessed_data_.has_value() ? public_preprocessed_data_->common_data : *common_data_,
                        proof,
                        *table_description_,
                        *constraint_system_,
                        *lpc_scheme_
                    );

                if (verification_result) {
                    BOOST_LOG_TRIVIAL(info) << "Proof is verified";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Proof verification failed";
                }

                return verification_result;
            }

            bool read_circuit(const boost::filesystem::path& circuit_file_) {
                BOOST_LOG_TRIVIAL(info) << "Read circuit from " << circuit_file_;

                using ZkConstraintSystem = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintField>;
                using ConstraintMarshalling =
                    nil::crypto3::marshalling::types::plonk_constraint_system<TTypeBase, ZkConstraintSystem>;

                auto marshalled_value = detail::decode_marshalling_from_file<ConstraintMarshalling>(circuit_file_);
                if (!marshalled_value) {
                    return false;
                }
                constraint_system_.emplace(
                    nil::crypto3::marshalling::types::make_plonk_constraint_system<Endianness, ZkConstraintSystem>(
                        *marshalled_value
                    )
                );

                return true;
            }

            bool save_circuit_to_file(boost::filesystem::path circuit_file) {
                using writer = circuit_writer<Endianness, BlueprintField>;

                BOOST_LOG_TRIVIAL(info) << "Writing circuit to " << circuit_file;
                if (!constraint_system_) {
                    BOOST_LOG_TRIVIAL(error) << "No circuit is currently loaded";
                    return false;
                }

                std::ofstream out(circuit_file.string(), std::ios::binary | std::ios::out);
                if (!out.is_open()) {
                    BOOST_LOG_TRIVIAL(error) << "Failed to open file " << circuit_file;
                    return false;
                }

                writer::write_binary_circuit(out, *constraint_system_, constraint_system_->public_input_sizes());
                return true;
            }

            bool set_circuit(const ConstraintSystem& circuit) {
                BOOST_LOG_TRIVIAL(info) << "Set circuit" << std::endl;

                constraint_system_.emplace(std::move(circuit));
                return true;
            }

            bool read_assignment_table(const boost::filesystem::path& assignment_table_file_path) {
                BOOST_LOG_TRIVIAL(info) << "Read assignment table from " << assignment_table_file_path;

                auto marshalled_table =
                    detail::decode_marshalling_from_file<TableMarshalling>(assignment_table_file_path);
                if (!marshalled_table) {
                    return false;
                }

                auto [table_description, assignment_table] =
                    nil::crypto3::marshalling::types::make_assignment_table<Endianness, AssignmentTable>(
                        *marshalled_table
                    );
                table_description_.emplace(table_description);
                assignment_table_.emplace(std::move(assignment_table));
                public_inputs_.emplace(assignment_table_->public_inputs());

                return true;
            }

            bool set_assignment_table(const AssignmentTable& assignment_table, std::size_t used_rows_amount) {
                BOOST_LOG_TRIVIAL(info) << "Set external assignment table" << std::endl;

                table_description_->witness_columns = assignment_table.witnesses_amount();
                table_description_->public_input_columns = assignment_table.public_inputs_amount();
                table_description_->constant_columns = assignment_table.constants_amount();
                table_description_->selector_columns = assignment_table.selectors_amount();
                table_description_->usable_rows_amount = used_rows_amount;
                table_description_->rows_amount = assignment_table.rows_amount();
                assignment_table_.emplace(std::move(assignment_table));
                public_inputs_.emplace(assignment_table_->public_inputs());
                return true;
            }

            bool save_binary_assignment_table_to_file(const boost::filesystem::path& output_filename) {
                using writer = assignment_table_writer<Endianness, BlueprintField>;

                BOOST_LOG_TRIVIAL(info) << "Writing binary assignment table to " << output_filename;

                if (!assignment_table_.has_value() || !table_description_.has_value()) {
                    BOOST_LOG_TRIVIAL(error) << "No assignment table is currently loaded";
                    return false;
                }

                std::ofstream out(output_filename.string(), std::ios::binary | std::ios::out);
                if (!out.is_open()) {
                    BOOST_LOG_TRIVIAL(error) << "Failed to open file " << output_filename;
                    return false;
                }

                writer::write_binary_assignment(
                    out, assignment_table_.value(), table_description_.value()
                );

                return true;
            }

            bool print_debug_assignment_table(const OutputArtifacts& opts) {
                if (opts.empty()) {
                    BOOST_LOG_TRIVIAL(trace) << "No output artifacts are set";
                    return true;
                }

                if (!assignment_table_.has_value() || !table_description_.has_value()) {
                    BOOST_LOG_TRIVIAL(error) << "No assignment table is currently loaded";
                    return false;
                }

                BOOST_LOG_TRIVIAL(debug) << "Rows to print: " << opts.rows.to_string();
                BOOST_LOG_TRIVIAL(debug) << "Witness columns to print: "
                                         << opts.witness_columns.to_string();
                BOOST_LOG_TRIVIAL(debug) << "Public input columns to print: "
                                         << opts.public_input_columns.to_string();
                BOOST_LOG_TRIVIAL(debug) << "Constant columns to print: "
                                         << opts.constant_columns.to_string();
                BOOST_LOG_TRIVIAL(debug) << "Selector columns to print: "
                                         << opts.selector_columns.to_string();


                const auto write = [&](std::ostream& out) -> bool {
                    return assignment_table_writer<Endianness, BlueprintField>::write_text_assignment(
                        out,
                        assignment_table_.value(),
                        table_description_.value(),
                        opts
                    );
                };

                if (opts.to_stdout()) {
                    BOOST_LOG_TRIVIAL(info) << "Writing text assignment table to stdout";
                    return write(std::cout);
                }

                BOOST_LOG_TRIVIAL(info) << "Writing text assignment table to " << opts.output_filename;
                std::ofstream out(opts.output_filename, std::ios::binary | std::ios::out);
                if (!out.is_open()) {
                    BOOST_LOG_TRIVIAL(error) << "Failed to open file " << opts.output_filename;
                    return false;
                }

                return write(out);
            }

            bool save_assignment_description(const boost::filesystem::path& assignment_description_file) {
                BOOST_LOG_TRIVIAL(info) << "Writing assignment description to " << assignment_description_file;

                auto marshalled_assignment_description =
                    nil::crypto3::marshalling::types::fill_assignment_table_description<Endianness, BlueprintField>(
                        *table_description_
                    );
                bool res = detail::encode_marshalling_to_file(
                    assignment_description_file,
                    marshalled_assignment_description
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Assignment description written.";
                }
                return res;
            }

            bool read_assignment_description(const boost::filesystem::path& assignment_description_file_) {
                BOOST_LOG_TRIVIAL(info) << "Read assignment description from " << assignment_description_file_;

                using TableDescriptionMarshalling =
                    nil::crypto3::marshalling::types::plonk_assignment_table_description<TTypeBase>;
                auto marshalled_description =
                    detail::decode_marshalling_from_file<TableDescriptionMarshalling>(assignment_description_file_);
                if (!marshalled_description) {
                    return false;
                }
                auto table_description =
                    nil::crypto3::marshalling::types::make_assignment_table_description<Endianness, BlueprintField>(
                        *marshalled_description
                    );
                table_description_.emplace(table_description);
                return true;
           }

            std::optional<typename BlueprintField::value_type> read_challenge(
                    const boost::filesystem::path& input_file) {
                using challenge_marshalling_type = nil::crypto3::marshalling::types::field_element<
                    TTypeBase, typename BlueprintField::value_type>;

                if (!can_read_from_file(input_file.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't read file " << input_file;
                    return std::nullopt;
                }

                auto marshalled_challenge = detail::decode_marshalling_from_file<challenge_marshalling_type>(
                    input_file);

                if (!marshalled_challenge) {
                    return std::nullopt;
                }

                return marshalled_challenge->value();
            }

            bool save_challenge(const boost::filesystem::path& challenge_file,
                                const typename BlueprintField::value_type& challenge) {
                using challenge_marshalling_type = nil::crypto3::marshalling::types::field_element<
                    TTypeBase, typename BlueprintField::value_type>;

                BOOST_LOG_TRIVIAL(info) << "Writing challenge to " << challenge_file;

                // marshall the challenge
                challenge_marshalling_type marshalled_challenge(challenge);

                return detail::encode_marshalling_to_file<challenge_marshalling_type>
                    (challenge_file, marshalled_challenge);
            }

            void create_lpc_scheme() {
                // Lambdas and grinding bits should be passed through preprocessor directives
                std::size_t table_rows_log = std::ceil(std::log2(table_description_->rows_amount));

                lpc_scheme_.emplace(FriParams(1, table_rows_log, lambda_, expand_factor_, grind_!=0, grind_));
            }

            bool preprocess_public_data() {
                public_inputs_.emplace(assignment_table_->public_inputs());

                create_lpc_scheme();

                BOOST_LOG_TRIVIAL(info) << "Preprocessing public data";
                TIME_LOG_SCOPE("Preprocess Public Data")
                public_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::
                        process(
                            *constraint_system_,
                            assignment_table_->move_public_table(),
                            *table_description_,
                            *lpc_scheme_,
                            max_quotient_chunks_
                        )
                );
                return true;
            }

            bool preprocess_private_data() {

                BOOST_LOG_TRIVIAL(info) << "Preprocessing private data";
                TIME_LOG_SCOPE("Preprocess Private Data")
                private_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::
                        process(*constraint_system_, assignment_table_->move_private_table(), *table_description_)
                );
                BOOST_LOG_TRIVIAL(info) << "Preprocess private data";

                // This is the last stage of preprocessor, and the assignment table is not used after this function call.
                assignment_table_.reset();

                return true;
            }

            bool generate_aggregated_challenge_to_file(
                const std::vector<boost::filesystem::path> &aggregate_input_files,
                const boost::filesystem::path &aggregated_challenge_file
            ) {
                if (aggregate_input_files.empty()) {
                    BOOST_LOG_TRIVIAL(error) << "No input files for challenge aggregation";
                    return false;
                }
                BOOST_LOG_TRIVIAL(info) << "Generating aggregated challenge to " << aggregated_challenge_file;

                // create the transcript
                using transcript_hash_type = typename PlaceholderParams::transcript_hash_type;
                using transcript_type = crypto3::zk::transcript::fiat_shamir_heuristic_sequential<transcript_hash_type>;
                transcript_type transcript;

                // read challenges from input files and add them to the transcript
                for (const auto &input_file : aggregate_input_files) {
                    std::optional<typename BlueprintField::value_type> challenge = read_challenge(input_file);
                    if (!challenge) {
                        return false;
                    }
                    transcript(challenge.value());
                }

                // produce the aggregated challenge
                auto output_challenge = transcript.template challenge<BlueprintField>();

                return save_challenge(aggregated_challenge_file, output_challenge);
            }

            // NOTE: PolynomialType is not required to match polynomial_type.
            template <typename PolynomialType>
            bool save_poly_to_file(const PolynomialType& poly,
                                   const boost::filesystem::path &output_file) {
                using polynomial_marshalling_type = typename nil::crypto3::marshalling::types::polynomial<
                    TTypeBase, PolynomialType>::type;

                BOOST_LOG_TRIVIAL(info) << "Writing polynomial to " << output_file;

                polynomial_marshalling_type marshalled_poly = nil::crypto3::marshalling::types::fill_polynomial<Endianness, PolynomialType>(poly);

                return detail::encode_marshalling_to_file<polynomial_marshalling_type>(
                    output_file, marshalled_poly);
            }

            // NOTE: PolynomialType is not required to match polynomial_type.
            template <typename PolynomialType>
            std::optional<PolynomialType> read_poly_from_file(const boost::filesystem::path &input_file) {
                using polynomial_marshalling_type = typename nil::crypto3::marshalling::types::polynomial<
                    TTypeBase, PolynomialType>::type;

                if (!can_read_from_file(input_file.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't read file " << input_file;
                    return std::nullopt;
                }

                auto marshalled_poly = detail::decode_marshalling_from_file<polynomial_marshalling_type>(
                    input_file);

                if (!marshalled_poly) {
                    BOOST_LOG_TRIVIAL(error) << "Problem with de-marshalling a polynomial read from a file" << input_file;
                    return std::nullopt;
                }

                return nil::crypto3::marshalling::types::make_polynomial<Endianness, PolynomialType>(marshalled_poly.value());
            }

            bool generate_combined_Q_to_file(
                const boost::filesystem::path &aggregated_challenge_file,
                std::size_t starting_power,
                const boost::filesystem::path &output_combined_Q_file) {
                std::optional<typename BlueprintField::value_type> challenge = read_challenge(
                    aggregated_challenge_file);
                if (!challenge) {
                    return false;
                }

                BOOST_LOG_TRIVIAL(info) << "Generating combined Q from " << aggregated_challenge_file
                    << " to " << output_combined_Q_file << " with starting_power " << starting_power;

                polynomial_type combined_Q = lpc_scheme_->prepare_combined_Q(
                    challenge.value(), starting_power);
                return save_poly_to_file(combined_Q, output_combined_Q_file);
            }

            bool merge_proofs(
                const std::vector<boost::filesystem::path> &partial_proof_files,
                const std::vector<boost::filesystem::path> &initial_proof_files,
                const boost::filesystem::path &aggregated_FRI_file,
                const boost::filesystem::path &merged_proof_file)
            {
                /* ZK types */
                using placeholder_aggregated_proof_type = nil::crypto3::zk::snark::
                    placeholder_aggregated_proof<BlueprintField, PlaceholderParams>;

                using partial_proof_type = Proof;

                using initial_proof_type = typename LpcScheme::lpc_proof_type;

                /* Marshalling types */
                using partial_proof_marshalled_type = nil::crypto3::marshalling::types::
                    placeholder_proof<nil::crypto3::marshalling::field_type<Endianness>, Proof>;

                using initial_proof_marshalling_type = nil::crypto3::marshalling::types::
                    inital_eval_proof<TTypeBase, LpcScheme>;

                using fri_proof_marshalling_type = nil::crypto3::marshalling::types::
                    initial_fri_proof_type<TTypeBase, LpcScheme>;

                using merged_proof_marshalling_type = nil::crypto3::marshalling::types::
                    placeholder_aggregated_proof_type<TTypeBase, placeholder_aggregated_proof_type>;


                placeholder_aggregated_proof_type merged_proof;

                if (partial_proof_files.size() != initial_proof_files.size() ) {
                    BOOST_LOG_TRIVIAL(error) << "Number of partial and initial proof files should match.";
                    return false;
                }

                for(auto const& partial_proof_file: partial_proof_files) {
                    BOOST_LOG_TRIVIAL(info) << "Reading partial proof from file \"" << partial_proof_file << "\"";
                    auto marshalled_partial_proof = detail::decode_marshalling_from_file<partial_proof_marshalled_type>(partial_proof_file, true);
                    if (!marshalled_partial_proof) {
                        BOOST_LOG_TRIVIAL(error) << "Error reading partial_proof from from \"" << partial_proof_file << "\"";
                        return false;
                    }

                    partial_proof_type partial_proof = nil::crypto3::marshalling::types::
                        make_placeholder_proof<Endianness, Proof>(*marshalled_partial_proof);

                    merged_proof.partial_proofs.emplace_back(partial_proof);
                }

                for(auto const& initial_proof_file: initial_proof_files) {

                    BOOST_LOG_TRIVIAL(info) << "Reading initial proof from file \"" << initial_proof_file << "\"";
                    auto initial_proof =
                        detail::decode_marshalling_from_file<initial_proof_marshalling_type>(initial_proof_file);
                    if (!initial_proof) {
                        BOOST_LOG_TRIVIAL(error) << "Error reading lpc_consistency_proof from \"" << initial_proof_file << "\"";
                    }

                    merged_proof.aggregated_proof.initial_proofs_per_prover.emplace_back(
                        nil::crypto3::marshalling::types::make_initial_eval_proof<Endianness, LpcScheme>(*initial_proof)
                    );
                }

                BOOST_LOG_TRIVIAL(info) << "Reading aggregated FRI proof from file \"" << aggregated_FRI_file << "\"";

                auto marshalled_fri_proof = detail::decode_marshalling_from_file<fri_proof_marshalling_type>(aggregated_FRI_file);

                if (!marshalled_fri_proof) {
                    BOOST_LOG_TRIVIAL(error) << "Error reading fri_proof from \"" << aggregated_FRI_file << "\"";
                    return false;
                }
                merged_proof.aggregated_proof.fri_proof =
                    nil::crypto3::marshalling::types::make_initial_fri_proof<Endianness, LpcScheme>(*marshalled_fri_proof);

                BOOST_LOG_TRIVIAL(info) << "Writing merged proof to \"" << merged_proof_file << "\"";

                auto marshalled_proof = nil::crypto3::marshalling::types::fill_placeholder_aggregated_proof
                    <Endianness, placeholder_aggregated_proof_type, partial_proof_type>
                    (merged_proof, lpc_scheme_->get_fri_params());

                return detail::encode_marshalling_to_file<merged_proof_marshalling_type>(merged_proof_file, marshalled_proof);
            }

            bool save_fri_proof_to_file(
                    const typename LpcScheme::fri_proof_type& fri_proof,
                    const boost::filesystem::path &output_file) {
                using fri_proof_marshalling_type = nil::crypto3::marshalling::types::initial_fri_proof_type<
                    TTypeBase, LpcScheme>;

                BOOST_LOG_TRIVIAL(info) << "Writing aggregated FRI proof to " << output_file;

                fri_proof_marshalling_type marshalled_proof = nil::crypto3::marshalling::types::fill_initial_fri_proof<Endianness, LpcScheme>(fri_proof);

                return detail::encode_marshalling_to_file<fri_proof_marshalling_type>(
                    output_file, marshalled_proof);
            }

            bool save_proof_of_work(
                const typename FriType::grinding_type::output_type &proof_of_work,
                const boost::filesystem::path &output_file) {
                using POW_marshalling_type = nil::crypto3::marshalling::types::integral<TTypeBase, typename FriType::grinding_type::output_type>;
                BOOST_LOG_TRIVIAL(info) << "Writing proof of work to " << output_file;

                POW_marshalling_type marshalled_pow(proof_of_work);

                return detail::encode_marshalling_to_file<POW_marshalling_type>(
                    output_file, marshalled_pow);
            }

            bool save_challenge_vector_to_file(
                const std::vector<typename BlueprintField::value_type>& challenges,
                const boost::filesystem::path& consistency_checks_challenges_output_file) {

                using challenge_vector_marshalling_type = nil::crypto3::marshalling::types::field_element_vector<
                    typename BlueprintField::value_type, TTypeBase>;

                BOOST_LOG_TRIVIAL(info) << "Writing challenges to " << consistency_checks_challenges_output_file;

                challenge_vector_marshalling_type marshalled_challenges =
                    nil::crypto3::marshalling::types::fill_field_element_vector<typename BlueprintField::value_type, Endianness>(
                        challenges);

                return detail::encode_marshalling_to_file<challenge_vector_marshalling_type>(
                    consistency_checks_challenges_output_file, marshalled_challenges);
            }

            std::optional<std::vector<typename BlueprintField::value_type>> read_challenge_vector_from_file(
                const boost::filesystem::path& input_file) {

                using challenge_vector_marshalling_type = nil::crypto3::marshalling::types::field_element_vector<
                    typename BlueprintField::value_type, TTypeBase>;

                if (!can_read_from_file(input_file.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't read file " << input_file;
                    return std::nullopt;
                }

                auto marshalled_challenges = detail::decode_marshalling_from_file<challenge_vector_marshalling_type>(
                    input_file);

                if (!marshalled_challenges) {
                    return std::nullopt;
                }

                return nil::crypto3::marshalling::types::make_field_element_vector<
                    typename BlueprintField::value_type, Endianness>(marshalled_challenges.value());
            }

            bool generate_aggregated_FRI_proof_to_file(
                const boost::filesystem::path &aggregated_challenge_file,
                const std::vector<boost::filesystem::path>& input_combined_Q_polynomial_files,
                const boost::filesystem::path& aggregated_fri_proof_output_file,
                const boost::filesystem::path& proof_of_work_output_file,
                const boost::filesystem::path& consistency_checks_challenges_output_file) {

                std::optional<typename BlueprintField::value_type> aggregated_challenge = read_challenge(
                    aggregated_challenge_file);
                if (!aggregated_challenge) {
                    return false;
                }

                // create the transcript
                using transcript_hash_type = typename PlaceholderParams::transcript_hash_type;
                using transcript_type = crypto3::zk::transcript::fiat_shamir_heuristic_sequential<transcript_hash_type>;
                transcript_type transcript;

                transcript(aggregated_challenge.value());

                // Sum up all the polynomials from the files.
                polynomial_type sum_poly;
                for (const auto& path : input_combined_Q_polynomial_files) {
                    std::optional<polynomial_type> next_combined_Q = read_poly_from_file<polynomial_type>(path);
                    if (!next_combined_Q) {
                        return false;
                    }
                    sum_poly += next_combined_Q.value();
                }
                create_lpc_scheme();
                auto [fri_proof, challenges] = lpc_scheme_->proof_eval_FRI_proof(sum_poly, transcript);

                // And finally run proof of work.
                typename FriType::grinding_type::output_type proof_of_work = nil::crypto3::zk::algorithms::run_grinding<FriType>(
                    lpc_scheme_->get_fri_params(), transcript);

                return save_fri_proof_to_file(fri_proof, aggregated_fri_proof_output_file) &&
                    save_proof_of_work(proof_of_work, proof_of_work_output_file) &&
                    save_challenge_vector_to_file(challenges, consistency_checks_challenges_output_file);
            }

            bool save_lpc_consistency_proof_to_file(
                    const typename LpcScheme::lpc_proof_type& lpc_consistency_proof,
                    const boost::filesystem::path &output_file) {
                // TODO(martun): consider changinge the class name 'inital_eval_proof'.
                using lpc_consistency_proof_marshalling_type = nil::crypto3::marshalling::types::inital_eval_proof<
                    TTypeBase, LpcScheme>;

                BOOST_LOG_TRIVIAL(info) << "Writing LPC consistency proof to " << output_file;

                lpc_consistency_proof_marshalling_type marshalled_proof = nil::crypto3::marshalling::types::fill_initial_eval_proof<Endianness, LpcScheme>(lpc_consistency_proof);

                return detail::encode_marshalling_to_file<lpc_consistency_proof_marshalling_type>(
                    output_file, marshalled_proof);
            }

            bool generate_consistency_checks_to_file(
                const boost::filesystem::path& combined_Q_file,
                const boost::filesystem::path& consistency_checks_challenges_output_file,
                const boost::filesystem::path& output_proof_file) {

                std::optional<std::vector<typename BlueprintField::value_type>> challenges = read_challenge_vector_from_file(
                    consistency_checks_challenges_output_file);
                if (!challenges)
                    return false;

                std::optional<polynomial_type> combined_Q = read_poly_from_file<polynomial_type>(combined_Q_file);
                if (!combined_Q)
                    return false;

                typename LpcScheme::lpc_proof_type proof = lpc_scheme_->proof_eval_lpc_proof(
                    combined_Q.value(), challenges.value());

                return save_lpc_consistency_proof_to_file(proof, output_proof_file);
            }

            bool setup_prover(const CircuitsLimits& circuits_limits) {
                TIME_LOG_SCOPE("Preset")
                const auto err = CircuitFactory<BlueprintField>::initialize_circuit(circuit_name_, constraint_system_, assignment_table_, table_description_, circuits_limits);
                if (err) {
                    BOOST_LOG_TRIVIAL(error) << "Can't initialize circuit " << circuit_name_ << ": " << err.value();
                    return false;
                }
                return true;
            }

            const ConstraintSystem& get_constraint_system() const {
                BOOST_ASSERT(constraint_system_);
                return constraint_system_.value();
            }

            const AssignmentTable& get_assignment_table() const {
                BOOST_ASSERT(assignment_table_);
                return assignment_table_.value();
            }

            bool fill_assignment_table(const boost::filesystem::path& trace_base_path, const AssignerOptions& options) {
                if (!constraint_system_.has_value()) {
                    BOOST_LOG_TRIVIAL(error) << "Circuit is not initialized";
                    return false;
                }
                if (!assignment_table_.has_value()) {
                    BOOST_LOG_TRIVIAL(error) << "Assignment table is not initialized";
                    return false;
                }
                TIME_LOG_SCOPE("Fill Assignment Table")
                const auto err = fill_assignment_table_single_thread(*assignment_table_, *table_description_, circuit_name_, trace_base_path, options);
                if (err) {
                    BOOST_LOG_TRIVIAL(error) << "Can't fill assignment table from trace " << trace_base_path << ": " << err.value();
                    return false;
                }
                return true;
            }

        private:
            const std::size_t expand_factor_;
            const std::size_t max_quotient_chunks_;
            const std::size_t lambda_;
            const std::size_t grind_;
            const std::string circuit_name_;

            std::optional<PublicPreprocessedData> public_preprocessed_data_;

            // TODO: This is used in verifier, since it does not need the whole preprocessed data.
            // It makes sence to separate prover class from verifier later.
            std::optional<CommonData> common_data_;

            std::optional<PrivatePreprocessedData> private_preprocessed_data_;
            std::optional<typename AssignmentTable::public_input_container_type> public_inputs_;
            std::optional<TableDescription> table_description_;
            std::optional<ConstraintSystem> constraint_system_;
            std::optional<AssignmentTable> assignment_table_;
            std::optional<LpcScheme> lpc_scheme_;
        };

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ASSIGNER_PROOF_HPP
