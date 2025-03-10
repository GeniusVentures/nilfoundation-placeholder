#ifndef PROOF_GENERATOR_LIBS_ASSIGNER_RW_HPP_
#define PROOF_GENERATOR_LIBS_ASSIGNER_RW_HPP_

#include <optional>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/assignment.hpp>
#include <nil/blueprint/zkevm_bbf/rw.hpp>
#include <nil/proof-generator/assigner/options.hpp>
#include <nil/proof-generator/assigner/trace_parser.hpp>

namespace nil {
    namespace proof_generator {

        /// @brief Fill assignment table
        template<typename BlueprintFieldType>
        std::optional<std::string> fill_rw_assignment_table(nil::crypto3::zk::snark::plonk_assignment_table<BlueprintFieldType>& assignment_table,
                                                            const boost::filesystem::path& trace_base_path,
                                                            const AssignerOptions& options) {
            BOOST_LOG_TRIVIAL(debug) << "fill rw table from " << trace_base_path << "\n";

            using ComponentType = nil::blueprint::bbf::rw<BlueprintFieldType, nil::blueprint::bbf::GenerationStage::ASSIGNMENT>;

            typename nil::blueprint::bbf::context<BlueprintFieldType, nil::blueprint::bbf::GenerationStage::ASSIGNMENT> context_object(assignment_table, options.circuits_limits.max_rows);

            const auto rw_trace_path = get_rw_trace_path(trace_base_path);
            auto input = deserialize_rw_traces_from_file(rw_trace_path, options);
            if (!input) {
                return "can't read rw from file: " + rw_trace_path.string();
            }

            ComponentType instance(context_object, input->value, options.circuits_limits.max_rw_size, options.circuits_limits.max_mpt_size);

            return {};
        }
    } // proof_generator
} // nil

#endif  // PROOF_GENERATOR_LIBS_ASSIGNER_RW_HPP_
