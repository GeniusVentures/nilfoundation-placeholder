#ifndef PROOF_GENERATOR_LIBS_ASSIGNER_KECCAK_HPP_
#define PROOF_GENERATOR_LIBS_ASSIGNER_KECCAK_HPP_

#include <optional>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/assignment.hpp>
#include <nil/blueprint/zkevm_bbf/keccak.hpp>
#include <nil/proof-generator/assigner/options.hpp>
#include <nil/proof-generator/assigner/trace_parser.hpp>

namespace nil {
    namespace proof_producer {

        /// @brief Fill assignment table
        template<typename BlueprintFieldType>
        std::optional<std::string> fill_keccak_assignment_table(nil::crypto3::zk::snark::plonk_assignment_table<BlueprintFieldType>& assignment_table,
                                                             const boost::filesystem::path& trace_base_path,
                                                             const AssignerOptions& options) {
            BOOST_LOG_TRIVIAL(debug) << "fill keccak table from " << trace_base_path << "\n";

            using ComponentType = nil::blueprint::bbf::keccak<BlueprintFieldType, nil::blueprint::bbf::GenerationStage::ASSIGNMENT>;

            typename nil::blueprint::bbf::context<BlueprintFieldType, nil::blueprint::bbf::GenerationStage::ASSIGNMENT> context_object(assignment_table, options.circuits_limits.max_rows);

            typename ComponentType::input_type input;
            input.rlc_challenge = options.circuits_limits.RLC_CHALLENGE;

            const auto keccak_trace_path = get_keccak_trace_path(trace_base_path);
            const auto keccak_operations = deserialize_keccak_traces_from_file(keccak_trace_path, options);
            if (!keccak_operations) {
                return "can't read keccak operations from file: " + keccak_trace_path.string();
            }
            // TODO(oclaw) adjust input
            // input = std::move(keccak_operations->value);

            ComponentType instance(
                context_object,
                input
                // ,options.circuits_limits.max_rows,
                // options.circuits_limits.max_keccak_blocks
            );

            return {};
        }
    } // proof_producer
} // nil

#endif  // PROOF_GENERATOR_LIBS_ASSIGNER_KECCAK_HPP_
