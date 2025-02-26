#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <nil/blueprint/utils/satisfiability_check.hpp>
#include <nil/proof-generator/commands/preset_command.hpp>
#include <nil/proof-generator/commands/fill_assignment_command.hpp>


namespace {

    struct Input {
        std::string trace_base_name; // base name of trace set collected from the cluster
        std::string circuit_name;    // circuit name
        bool skip_check{false};      // skip satisfiability check while running the test
    };

} // namespace


class ProverTests: public ::testing::TestWithParam<Input> {

    public:
        using CurveType = nil::crypto3::algebra::curves::pallas;
        using HashType = nil::crypto3::hashes::keccak_1600<256>;

        using ConstraintSystem = nil::proof_producer::TypeSystem<CurveType, HashType>::ConstraintSystem;
        using BlueprintFieldType = nil::proof_producer::TypeSystem<CurveType, HashType>::BlueprintField;
        using AssignmentTable = nil::proof_producer::TypeSystem<CurveType, HashType>::AssignmentTable;

        class AssignmentTableChecker: public nil::proof_producer::command_chain {

        public:
            AssignmentTableChecker(const std::string& circuit_name, const std::string& trace_base_path) {
                using PresetStep                       = typename nil::proof_producer::PresetStep<CurveType, HashType>::Executor;
                using Assigner                         = typename nil::proof_producer::FillAssignmentStep<CurveType, HashType>::Executor;

                nil::proof_producer::CircuitsLimits circuit_limits;
                auto& circuit_maker = add_step<PresetStep>(circuit_name, circuit_limits);
                auto& assigner = add_step<Assigner>(circuit_maker, circuit_maker, circuit_name, trace_base_path,
                    nil::proof_producer::AssignerOptions(false, circuit_limits));

                resources::subscribe_value<ConstraintSystem>(circuit_maker, circuit_);    // capture circuit to do the check
                resources::subscribe_value<AssignmentTable>(assigner, assignment_table_); // capture assignment table to do the check
            }

            std::shared_ptr<ConstraintSystem> circuit_;
            std::shared_ptr<AssignmentTable> assignment_table_;
        };
};


TEST_P(ProverTests, FillAssignmentAndCheck) {
    const auto input = GetParam();
    const std::string trace_base_path = std::string(TEST_DATA_DIR) + input.trace_base_name;

    AssignmentTableChecker checker(input.circuit_name, trace_base_path);
    auto const res = checker.execute();
    ASSERT_TRUE(res.succeeded());

    if (input.skip_check) {
        GTEST_SKIP() << "Skipping satisfiability_check for " << input.circuit_name <<   " circuit for trace " << input.trace_base_name;
    }

    ASSERT_NE(checker.circuit_, nullptr);
    ASSERT_NE(checker.assignment_table_, nullptr);

    auto const check_res = nil::blueprint::satisfiability_checker<BlueprintFieldType>::is_satisfied(
        *checker.circuit_,
        *checker.assignment_table_,
        nil::blueprint::satisfiability_check_options{.verbose = true}
    );

    ASSERT_TRUE(check_res);
}

using namespace nil::proof_producer::circuits;

// !! note that due to https://github.com/NilFoundation/placeholder/issues/196
// contracts for these traces were compiled with --no-cbor-metadata flag

// Single call of SimpleStorage contract increment + keccakHash functions
const std::string SimpleIncAndKeccak = "simple/simple_inc_and_keccak";
INSTANTIATE_TEST_SUITE_P(SimpleRw, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak,  RW}));
INSTANTIATE_TEST_SUITE_P(SimpleBytecode, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak,  BYTECODE}));
INSTANTIATE_TEST_SUITE_P(SimpleCopy, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak,  COPY}));
INSTANTIATE_TEST_SUITE_P(SimpleZkevm, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak,  ZKEVM}));
INSTANTIATE_TEST_SUITE_P(SimpleExp, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak, EXP}));
INSTANTIATE_TEST_SUITE_P(SimpleKeccak, ProverTests, ::testing::Values(Input{SimpleIncAndKeccak, KECCAK}));

// Multiple calls of SimpleStorage contract increment function (several transactions)
const std::string MultiTxIncrement = "multi_tx/increment_multi_tx";
INSTANTIATE_TEST_SUITE_P(MultiTxRw, ProverTests, ::testing::Values(Input{MultiTxIncrement,  RW}));
INSTANTIATE_TEST_SUITE_P(MultiTxBytecode, ProverTests, :: testing::Values(Input{MultiTxIncrement,  BYTECODE}));
INSTANTIATE_TEST_SUITE_P(MultiTxCopy, ProverTests, ::testing::Values(Input{MultiTxIncrement,  COPY}));
INSTANTIATE_TEST_SUITE_P(MultiTxZkevm, ProverTests, ::testing::Values(Input{MultiTxIncrement,  ZKEVM}));
INSTANTIATE_TEST_SUITE_P(MultiTxExp, ProverTests, ::testing::Values(Input{MultiTxIncrement, EXP}));
INSTANTIATE_TEST_SUITE_P(MultiTxKeccak, ProverTests, ::testing::Values(Input{MultiTxIncrement, KECCAK}));

// Single call of exp operation
const std::string SimpleExp = "exp/exp";
INSTANTIATE_TEST_SUITE_P(SimpleExpRw, ProverTests, ::testing::Values(Input{SimpleExp, RW}));
INSTANTIATE_TEST_SUITE_P(SimpleExpBytecode, ProverTests, :: testing::Values(Input{SimpleExp, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(SimpleExpCopy, ProverTests, ::testing::Values(Input{SimpleExp, COPY}));
INSTANTIATE_TEST_SUITE_P(SimpleExpZkevm, ProverTests, ::testing::Values(Input{SimpleExp, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(SimpleExpExp, ProverTests, ::testing::Values(Input{SimpleExp, EXP}));
INSTANTIATE_TEST_SUITE_P(SimpleExpKeccak, ProverTests, ::testing::Values(Input{SimpleExp, KECCAK}));

// ARITHMETIC CORNER CASES (OVERFLOW, UNDERFLOW, DIVISION BY ZERO)

const std::string AdditionOverflow = "corner_cases/addition_overflow/addition_overflow";
INSTANTIATE_TEST_SUITE_P(AdditionOverflowRw, ProverTests, ::testing::Values(Input{AdditionOverflow, RW}));
INSTANTIATE_TEST_SUITE_P(AdditionOverflowBytecode, ProverTests, :: testing::Values(Input{AdditionOverflow, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(AdditionOverflowCopy, ProverTests, ::testing::Values(Input{AdditionOverflow, COPY}));
INSTANTIATE_TEST_SUITE_P(AdditionOverflowZkevm, ProverTests, ::testing::Values(Input{AdditionOverflow, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(AdditionOverflowExp, ProverTests, ::testing::Values(Input{AdditionOverflow, EXP}));
INSTANTIATE_TEST_SUITE_P(AdditionOverflowKeccak, ProverTests, ::testing::Values(Input{AdditionOverflow, KECCAK}));

const std::string SubstractionUnderflow = "corner_cases/substraction_underflow/substraction_underflow";
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowRw, ProverTests, ::testing::Values(Input{SubstractionUnderflow, RW}));
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowBytecode, ProverTests, :: testing::Values(Input{SubstractionUnderflow, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowCopy, ProverTests, ::testing::Values(Input{SubstractionUnderflow, COPY}));
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowZkevm, ProverTests, ::testing::Values(Input{SubstractionUnderflow, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowExp, ProverTests, ::testing::Values(Input{SubstractionUnderflow, EXP}));
INSTANTIATE_TEST_SUITE_P(SubstractionUnderflowKeccak, ProverTests, ::testing::Values(Input{SubstractionUnderflow, KECCAK}));

const std::string DivByZero = "corner_cases/division_by_zero/div_by_zero";
INSTANTIATE_TEST_SUITE_P(DivByZeroRw, ProverTests, ::testing::Values(Input{DivByZero, RW}));
INSTANTIATE_TEST_SUITE_P(DivByZeroBytecode, ProverTests, :: testing::Values(Input{DivByZero, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(DivByZeroCopy, ProverTests, ::testing::Values(Input{DivByZero, COPY}));
INSTANTIATE_TEST_SUITE_P(DivByZeroZkevm, ProverTests, ::testing::Values(Input{DivByZero, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(DivByZeroExp, ProverTests, ::testing::Values(Input{DivByZero, EXP}));
INSTANTIATE_TEST_SUITE_P(DivByZeroKeccak, ProverTests, ::testing::Values(Input{DivByZero, KECCAK}));

const std::string MultiplicationOverflow = "corner_cases/multiplication_overflow/mul_overflow";
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowRw, ProverTests, ::testing::Values(Input{MultiplicationOverflow, RW}));
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowBytecode, ProverTests, :: testing::Values(Input{MultiplicationOverflow, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowCopy, ProverTests, ::testing::Values(Input{MultiplicationOverflow, COPY}));
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowZkevm, ProverTests, ::testing::Values(Input{MultiplicationOverflow, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowExp, ProverTests, ::testing::Values(Input{MultiplicationOverflow, EXP}));
INSTANTIATE_TEST_SUITE_P(MultiplicationOverflowKeccak, ProverTests, ::testing::Values(Input{MultiplicationOverflow, KECCAK}));

const std::string ExponentiationOverflow = "corner_cases/exponentiation_overflow/exp_overflow";
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowRw, ProverTests, ::testing::Values(Input{ExponentiationOverflow, RW}));
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowBytecode, ProverTests, :: testing::Values(Input{ExponentiationOverflow, BYTECODE}));
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowCopy, ProverTests, ::testing::Values(Input{ExponentiationOverflow, COPY}));
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowZkevm, ProverTests, ::testing::Values(Input{ExponentiationOverflow, ZKEVM}));
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowExp, ProverTests, ::testing::Values(Input{ExponentiationOverflow, EXP}));
INSTANTIATE_TEST_SUITE_P(ExponentiationOverflowKeccak, ProverTests, ::testing::Values(Input{ExponentiationOverflow, KECCAK}));

// RW trace is picked from another trace set and has different trace_idx
TEST(ProverTest, TraceIndexMismatch) {
    const std::string trace_base_path = std::string(TEST_DATA_DIR) + "/broken_index/increment_simple";

    ProverTests::AssignmentTableChecker checker(ZKEVM, trace_base_path);
    auto const res = checker.execute();
    ASSERT_EQ(res.result_code(), nil::proof_producer::ResultCode::InvalidInput);
    ASSERT_NE(checker.circuit_, nullptr);        // circuit is filled
    ASSERT_EQ(checker.assignment_table_, nullptr); // assignment table is not filled
}

// Trace files contain different proto hash
TEST(ProverTest, DifferentProtoHash) {
    const std::string trace_base_path = std::string(TEST_DATA_DIR) + "/different_proto/increment_simple.pb";

    ProverTests::AssignmentTableChecker checker(ZKEVM, trace_base_path);
    auto const res = checker.execute();
    ASSERT_EQ(res.result_code(), nil::proof_producer::ResultCode::InvalidInput);
    ASSERT_NE(checker.circuit_, nullptr);        // circuit is filled
    ASSERT_EQ(checker.assignment_table_, nullptr); // assignment table is not filled
}
