//---------------------------------------------------------------------------//
// Copyright (c) 2024 Alexey Yashunsky <a.yashunsky@nil.foundation>
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
// @file Declaration of interfaces for PLONK BBF context & generic component classes
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_BLUEPRINT_PLONK_BBF_GENERIC_HPP
#define CRYPTO3_BLUEPRINT_PLONK_BBF_GENERIC_HPP

#include <functional>

#include <boost/log/trivial.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
// #include <nil/crypto3/zk/snark/arithmetization/plonk/copy_constraint.hpp> // NB: part of the previous include

#include <nil/blueprint/blueprint/plonk/assignment.hpp>
#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/component.hpp>
//#include <nil/blueprint/manifest.hpp>
#include <nil/blueprint/gate_id.hpp>

#include <nil/blueprint/bbf/bool_field.hpp>

namespace nil {
    namespace blueprint {
        namespace bbf {

            enum class GenerationStage { ASSIGNMENT = 0, CONSTRAINTS = 1 };

            enum column_type { witness, public_input, constant, COLUMN_TYPES };

            std::ostream &operator<<(std::ostream &os, const column_type &t) {
                std::map<column_type, std::string> type_map = {
                    {column_type::witness, "witness"},
                    {column_type::public_input, "public input"},
                    {column_type::constant, "constant"},
                    {column_type::COLUMN_TYPES, " "}
                };
                os << type_map[t];
                return os;
            }

            template<typename VariableType>
            class expression_is_variable_visitor : public boost::static_visitor<bool> {
            public:
                expression_is_variable_visitor() {}

                static bool is_var(const crypto3::math::expression<VariableType>& expr) {
                    expression_is_variable_visitor v = expression_is_variable_visitor();
                    return boost::apply_visitor(v, expr.get_expr());
                }

                bool operator()(const crypto3::math::term<VariableType>& term) {
                    return ((term.get_vars().size() == 1) && term.get_coeff().is_one());
                }

                bool operator()(const crypto3::math::pow_operation<VariableType>& pow) {
                    return false;
                }

                bool operator()(const crypto3::math::binary_arithmetic_operation<VariableType>& op) {
                    return false;
                }
            };

            template<typename VariableType>
            class expression_row_range_visitor : public boost::static_visitor<std::tuple<bool,int32_t,int32_t>> {
            public:
                expression_row_range_visitor() {}

                static std::tuple<bool,int32_t,int32_t> row_range(const crypto3::math::expression<VariableType>& expr) {
                    expression_row_range_visitor v = expression_row_range_visitor();
                    return boost::apply_visitor(v, expr.get_expr());
                }

                std::tuple<bool,int32_t,int32_t> operator()(const crypto3::math::term<VariableType>& term) {
                    bool has_vars = false;
                    int32_t min_row, max_row;

                    if (term.get_vars().size() > 0) {
                        has_vars = true;
                        min_row = term.get_vars()[0].rotation;
                        max_row = term.get_vars()[0].rotation;
                        for(std::size_t i = 1; i < term.get_vars().size(); i++) {
                            min_row = std::min(min_row, term.get_vars()[i].rotation);
                            max_row = std::max(max_row, term.get_vars()[i].rotation);
                        }
                    }
                    return {has_vars, min_row, max_row};
                }

                std::tuple<bool,int32_t,int32_t> operator()(const crypto3::math::pow_operation<VariableType>& pow) {
                    return boost::apply_visitor(*this, pow.get_expr().get_expr());
                }

                std::tuple<bool,int32_t,int32_t> operator()(const crypto3::math::binary_arithmetic_operation<VariableType>& op) {
                    auto [A_has_vars, A_min, A_max] = boost::apply_visitor(*this, op.get_expr_left().get_expr());
                    auto [B_has_vars, B_min, B_max] = boost::apply_visitor(*this, op.get_expr_right().get_expr());

                    if (!A_has_vars) {
                        return {B_has_vars, B_min, B_max};
                    }
                    if (!B_has_vars) {
                        return {A_has_vars, A_min, A_max};
                    }
                    return {true, std::min(A_min,B_min), std::max(A_max,B_max)};
                }
            };

            template<typename VariableType>
            class expression_relativize_visitor : public boost::static_visitor<crypto3::math::expression<VariableType>> {
            private:
                int32_t shift;
            public:
                expression_relativize_visitor(int32_t shift_) : shift(shift_) {}

                static crypto3::math::expression<VariableType>
                relativize(const crypto3::math::expression<VariableType>& expr, int32_t shift) {
                    expression_relativize_visitor v = expression_relativize_visitor(shift);
                    return boost::apply_visitor(v, expr.get_expr());
                }

                crypto3::math::expression<VariableType>
                operator()(const crypto3::math::term<VariableType>& term) {
                    std::vector<VariableType> vars = term.get_vars();

                    for(std::size_t i = 0; i < vars.size(); i++) {
                        vars[i].relative = true;
                        vars[i].rotation += shift;
                        if (std::abs(vars[i].rotation) > 1) {
                            BOOST_LOG_TRIVIAL(warning) << "Rotation exceeds 1 after relativization in term " << term << ".\n";
                        }
                    }

                    return crypto3::math::term<VariableType>(vars, term.get_coeff());
                }

                crypto3::math::expression<VariableType>
                operator()(const crypto3::math::pow_operation<VariableType>& pow) {
                    return crypto3::math::pow_operation<VariableType>(
                        boost::apply_visitor(*this, pow.get_expr().get_expr()),
                        pow.get_power());
                }

                crypto3::math::expression<VariableType>
                operator()(const crypto3::math::binary_arithmetic_operation<VariableType>& op) {
                    return crypto3::math::binary_arithmetic_operation<VariableType>(
                        boost::apply_visitor(*this, op.get_expr_left().get_expr()),
                        boost::apply_visitor(*this, op.get_expr_right().get_expr()),
                        op.get_op());
                }
            };

            template<typename FieldType>
            class basic_context {
                using bool_field = crypto3::algebra::fields::bool_field;
                using assignment_type = assignment<crypto3::zk::snark::plonk_constraint_system<FieldType>>;
                using allocation_log_type = assignment<crypto3::zk::snark::plonk_constraint_system<bool_field>>;

                private:
                    std::shared_ptr<allocation_log_type> al;
                protected:
                    std::vector<std::size_t> col_map[COLUMN_TYPES];
                    std::size_t row_shift = 0; // united, for all column types
                    std::size_t max_rows;
                    std::size_t current_row[COLUMN_TYPES];

                public:
                    std::size_t get_col(std::size_t col, column_type t) {
                        if (col >= col_map[t].size()) {
                            BOOST_LOG_TRIVIAL(error) << "Column ("<< t <<") out of range ("<< col <<" >= " << (col_map[t].size()) << ").\n";
                        }
                        BOOST_ASSERT(col < col_map[t].size());
                        return col_map[t][col];
                    }
                    std::size_t get_row(std::size_t row) {
                        if (row >= max_rows) {
                            BOOST_LOG_TRIVIAL(error) << "Row out of range (" << row << " >= " << max_rows << ").\n";
                        }
                        BOOST_ASSERT(row < max_rows);
                        return row + row_shift;
                    }

                    bool is_allocated(std::size_t col, std::size_t row, column_type t) {
                        bool_field::value_type cell;
                        switch (t) {
                            case column_type::witness:      cell = al->witness(get_col(col,t),get_row(row)); break;
                            case column_type::public_input: cell = al->public_input(get_col(col,t),get_row(row)); break;
                            case column_type::constant:     cell = al->constant(get_col(col,t),get_row(row)); break;
                        }
                        return (cell == bool_field::value_type::one());
                    }

                    void print_witness_allocation_log() {
                        for(std::size_t j = 0; j < col_map[column_type::witness].size(); j++) {
                            std::cout << (j < 10 ? " " : "") << j << " ";
                        }
                        std::cout << "\n";
                        for(std::size_t i = 0; i < max_rows; i++) {
                            for(std::size_t j = 0; j < col_map[column_type::witness].size(); j++) {
                                std::cout << " " << (is_allocated(j,i,column_type::witness) ? "*" : "_") << " ";
                            }
                            std::cout << "\n";
                        }
                    }

                    void mark_allocated(std::size_t col, std::size_t row, column_type t) {
                        BOOST_ASSERT(row < max_rows);
                        switch (t) {
                            case column_type::witness:      al->witness(get_col(col,t),get_row(row)) = 1; break;
                            case column_type::public_input: al->public_input(get_col(col,t),get_row(row)) = 1; break;
                            case column_type::constant:     al->constant(get_col(col,t),get_row(row)) = 1; break;
                        }
                    }

                    std::pair<std::size_t, std::size_t> next_free_cell(column_type t) {
                        std::size_t col = 0,
                                    row = current_row[t],
                                    hsize = col_map[t].size();
                        bool found = false;

                        while((!found) && (current_row[t] < max_rows)) {
                            if (col >= hsize) {
                                current_row[t]++;
                                row = current_row[t];
                                col = 0;
                            }
                            found = !is_allocated(col,row,t);
                            if (!found) {
                                col++;
                            }
                        }

                        if (!found) {
                            BOOST_LOG_TRIVIAL(error) << "Insufficient space for allocation.\n";
                        }
                        BOOST_ASSERT(found);

                        return {col, row};
                    }

                    void new_line(column_type t) {
                        std::size_t col;
                        do {
                            auto [c, r] = next_free_cell(t);
                            col = c;
                            if (col > 0) {
                                current_row[t]++;
                            }
                        } while ((col > 0) && (current_row[t] < max_rows));

                        if (current_row[t] == max_rows) {
                            BOOST_LOG_TRIVIAL(error) << "Insufficient space for starting a new row.\n";
                        }
                        BOOST_ASSERT(col == 0);
                    }

                    basic_context(assignment_type &at, std::size_t max_rows_) :
                        current_row{0, 0, 0}, // For all types of columns start from 0. TODO: this might not be a good idea
                        max_rows(max_rows_)
                    {
                        al = std::make_shared<allocation_log_type>(at.witnesses_amount(),
                            at.public_inputs_amount(), at.constants_amount(), at.selectors_amount());
                        for(std::size_t i = 0; i < at.witnesses_amount(); i++) {
                            col_map[column_type::witness].push_back(i);
                        }
                        for(std::size_t i = 0; i < at.public_inputs_amount(); i++) {
                            col_map[column_type::public_input].push_back(i);
                        }
                        for(std::size_t i = 0; i < at.constants_amount(); i++) {
                            col_map[column_type::constant].push_back(i);
                        }
                    };
                    basic_context(assignment_type &at, std::size_t max_rows_, std::size_t row_shift_) :
                        basic_context(at,max_rows_) {
                        row_shift = row_shift_;
                    };
            };

            template<typename FieldType, GenerationStage stage> class context;

            template<typename FieldType>
            class context<FieldType, GenerationStage::ASSIGNMENT> : public basic_context<FieldType> { // assignment-specific definition
                using assignment_type = assignment<crypto3::zk::snark::plonk_constraint_system<FieldType>>;
                using plonk_copy_constraint = crypto3::zk::snark::plonk_copy_constraint<FieldType>;
                using lookup_constraint_type = std::pair<std::string,std::vector<typename FieldType::value_type>>;
                using basic_context<FieldType>::col_map;
                public:
                    using TYPE = typename FieldType::value_type;
                    using basic_context<FieldType>::get_col;
                    using basic_context<FieldType>::get_row;
                    using basic_context<FieldType>::is_allocated;
                    using basic_context<FieldType>::mark_allocated;

                private:
                    // reference to actual assignment table
                    assignment_type &at;

                public:
                    void allocate(TYPE &C, size_t col, size_t row, column_type t) {
                        if (is_allocated(col, row, t)) {
                            BOOST_LOG_TRIVIAL(warning) << "RE-allocation of " << t << " cell at col = " << col << ", row = " << row << ".\n";
                        }
                        switch (t) {
                            // NB: we use get_col/get_row here because active area might differ
                            // from the entire assignment table, while col and row are intended
                            // to be _relative_ to the active area
                            case column_type::witness:      at.witness(get_col(col,t), get_row(row)) = C;      break;
                            case column_type::public_input: at.public_input(get_col(col,t), get_row(row)) = C; break;
                            case column_type::constant:     at.constant(get_col(col,t), get_row(row)) = C;     break;
                        }
                        mark_allocated(col,row,t);
                    }
                    void copy_constrain(TYPE &A, TYPE &B) {
                        if (A != B) {
                            // NB: This might be an error, but we don't stop execution,
                            // because we want to be able to run tests-to-fail.
                            BOOST_LOG_TRIVIAL(warning) << "Assignment violates copy constraint (" << A << " != " << B << ")\n";
                        }
                    }
                    void constrain(TYPE C) {
                        if (C != 0) {
                            // NB: This might be an error, but we don't stop execution,
                            // because we want to be able to run tests-to-fail.
                            BOOST_LOG_TRIVIAL(warning) << "Assignment violates polynomial constraint (" << C << " != 0)\n";
                        }
                    }
                    void lookup(std::vector<TYPE> &C, std::string table_name) {
                        // TODO: actually check membership of C in table?
                    }

                    void optimize_gates() {
                        BOOST_LOG_TRIVIAL(error) << "optimize_gates() called at assignment stage.\n";
                    }
                    std::vector<std::pair<std::vector<TYPE>, std::set<std::size_t>>> get_constraints() {
                        BOOST_LOG_TRIVIAL(error) << "get_constraints() called at assignment stage.\n";
                        return {};
                    }
                    std::vector<plonk_copy_constraint> get_copy_constraints() {
                        BOOST_LOG_TRIVIAL(error) << "get_copy_constraints() called at assignment stage.\n";
                        return {};
                    }
                    std::vector<std::pair<std::vector<lookup_constraint_type>, std::set<std::size_t>>> get_lookup_constraints() {
                        BOOST_LOG_TRIVIAL(error) << "get_lookup_constraints() called at assignment stage.\n";
                        return {};
                    }

                    context subcontext(std::vector<std::size_t> W, std::size_t new_row_shift, std::size_t new_max_rows) {
                         context res = *this;
                         std::vector<std::size_t> new_W = {};
                         for(std::size_t i = 0; i < W.size(); i++) {
                             new_W.push_back(col_map[column_type::witness][W[i]]);
                         }
                         res.col_map[column_type::witness] = new_W;
                         res.row_shift += new_row_shift;
                         res.max_rows = new_max_rows;
                         res.current_row[column_type::witness] = 0; // reset to 0, because in the new column set everything is different
                         return res;
                    }

                    context(assignment_type &assignment_table, std::size_t max_rows) :
                         basic_context<FieldType>(assignment_table,max_rows), at(assignment_table)
                    { };

                    context(assignment_type &assignment_table, std::size_t max_rows, std::size_t row_shift) :
                         basic_context<FieldType>(assignment_table,max_rows,row_shift), at(assignment_table)
                    { };
            };

            template<typename FieldType>
            class context<FieldType, GenerationStage::CONSTRAINTS> : public basic_context<FieldType> { // circuit-specific definition
                using constraint_id_type = gate_id<FieldType>;
                using var = crypto3::zk::snark::plonk_variable<typename FieldType::value_type>;
                using constraint_type = crypto3::zk::snark::plonk_constraint<FieldType>;
                using plonk_copy_constraint = crypto3::zk::snark::plonk_copy_constraint<FieldType>;
                using constraints_container_type = std::map<constraint_id_type, std::pair<constraint_type, std::set<std::size_t>>>;
                using copy_constraints_container_type = std::vector<plonk_copy_constraint>; // TODO: maybe it's a set, not a vec?
                using lookup_constraints_container_type = std::map<std::pair<std::string,constraint_id_type>, // <table_name,expressions_id>
                                                                   std::pair<std::vector<constraint_type>,std::set<std::size_t>>>;
                                                                   // ^^^ expressions, rows
                using lookup_constraint_type = std::pair<std::string,std::vector<constraint_type>>; // NB: NOT exactly as plonk!!!
                using basic_context<FieldType>::col_map;

                using assignment_type = assignment<crypto3::zk::snark::plonk_constraint_system<FieldType>>;
                public:
                    using TYPE = constraint_type;
                    using basic_context<FieldType>::get_col;
                    using basic_context<FieldType>::get_row;
                    using basic_context<FieldType>::is_allocated;
                    using basic_context<FieldType>::mark_allocated;

                private:
                    // constraints (with unique id), and the rows they are applied to
                    std::shared_ptr<constraints_container_type> constraints;
                    // copy constraints as in BP
                    std::shared_ptr<copy_constraints_container_type> copy_constraints;
                    // lookup constraints with table name, unique id and row list
                    std::shared_ptr<lookup_constraints_container_type> lookup_constraints;

                    void add_constraint(TYPE &C_rel, std::size_t row) {
                        constraint_id_type C_id = constraint_id_type(C_rel);
                        if (constraints->find(C_id) != constraints->end()) {
                            constraints->at(C_id).second.insert(row);
                        } else {
                            constraints->insert({C_id, {C_rel, {row}}});
                        }
                    }
                    void add_lookup_constraint(std::string table_name, std::vector<TYPE> &C_rel, std::size_t row) {
                        constraint_id_type C_id = constraint_id_type(C_rel);
                        if (lookup_constraints->find({table_name,C_id}) != lookup_constraints->end()) {
                            lookup_constraints->at({table_name,C_id}).second.insert(row);
                        } else {
                            lookup_constraints->insert({{table_name,C_id}, {C_rel, {row}}});
                        }
                    }

                public:
                    void allocate(TYPE &C, size_t col, size_t row, column_type t) {
                        if (is_allocated(col, row, t)) {
                            BOOST_LOG_TRIVIAL(warning) << "RE-allocation of " << t << " cell at col = " << col << ", row = " << row << ".\n";
                        }
                        var res = var(get_col(col,t), get_row(row), // get_col/get_row are active-area-aware
                                      false, // false = use absolute cell address
                                      static_cast<typename var::column_type>(t));
                        if (C != TYPE()) { // TODO: is this ok?
                            constrain(res - C);
                        }
                        C = res;
                        mark_allocated(col,row,t);
                    }

                    void copy_constrain(TYPE &A, TYPE &B) {
                        auto is_var = expression_is_variable_visitor<var>::is_var;

                        if (!is_var(A) || !is_var(B)) {
                            BOOST_LOG_TRIVIAL(error) << "Copy constraint applied to non-variable: " << A << " = " << B << ".\n";
                        }
                        BOOST_ASSERT(is_var(A) && is_var(B));

                        var A_var = boost::get<crypto3::math::term<var>>(A.get_expr()).get_vars()[0];
                        var B_var = boost::get<crypto3::math::term<var>>(B.get_expr()).get_vars()[0];

                        if (A_var != B_var) {
                            copy_constraints->push_back({A_var,B_var});
                        }
                    }

                    void constrain(TYPE C) {
                        auto [has_vars, min_row, max_row] = expression_row_range_visitor<var>::row_range(C);
                        if (!has_vars) {
                            BOOST_LOG_TRIVIAL(error) << "Constraint " << C << " has no variables!\n";
                        }
                        BOOST_ASSERT(has_vars);
                        if (max_row - min_row > 2) {
                            BOOST_LOG_TRIVIAL(warning) << "Constraint " << C << " spans over 3 rows!\n";
                        }
                        std::size_t row = (min_row + max_row)/2;

                        TYPE C_rel = expression_relativize_visitor<var>::relativize(C, -row);
                        add_constraint(C_rel, row);
                    }

                    void lookup(std::vector<TYPE> &C, std::string table_name) {
                        std::set<std::size_t> base_rows = {};

                        // Choose the best row to relativize. Different expressions in a single lookup might accept
                        // up to 3 different rows for relativization. We take the intersection for all expressions in
                        // the constraint.
                        for(TYPE c_part : C) {
                            auto [has_vars, min_row, max_row] = expression_row_range_visitor<var>::row_range(c_part);
                            if (has_vars) { // NB: not having variables seems to be ok for a part of a lookup expression
                                if (max_row - min_row > 2) {
                                    BOOST_LOG_TRIVIAL(warning) << "Expression " << c_part << " in lookup constraint spans over 3 rows!\n";
                                }
                                std::size_t row = (min_row + max_row)/2;
                                std::set<std::size_t> current_base_rows = {row};
                                if (max_row - min_row <= 1) {
                                    current_base_rows.insert(row+1);
                                }
                                if (max_row == min_row) {
                                    current_base_rows.insert(row-1);
                                }
                                if (base_rows.empty()) {
                                    base_rows = current_base_rows;
                                } else {
                                    std::set<std::size_t> new_base_rows;
                                    std::set_intersection(base_rows.begin(), base_rows.end(),
                                                          current_base_rows.begin(), current_base_rows.end(),
                                                          std::inserter(new_base_rows, new_base_rows.end()));
                                    base_rows = new_base_rows;
                                }
                            }
                        }
                        if (base_rows.empty()) {
                            BOOST_LOG_TRIVIAL(error) << "Lookup constraint expressions have no variables or have incompatible spans!\n";
                        }
                        BOOST_ASSERT(!base_rows.empty());

                        std::size_t row = (base_rows.size() == 3) ? *(std::next(base_rows.begin())) : *(base_rows.begin());
                        std::vector<TYPE> res;
                        for(TYPE c_part : C) {
                            TYPE c_part_rel = expression_relativize_visitor<var>::relativize(c_part, -row);
                            res.push_back(c_part_rel);
                        }
                        add_lookup_constraint(table_name, res, row);
                    }

                    void optimize_gates() {
                        // NB: std::map<constraint_id_type, std::pair<constraint_type, std::set<std::size_t>>> constraints;
                        // intended to
                        // shift some of the constraints so that we have less selectors
                        /*
                        for(const auto& [id, data] : *constraints) {
                            std::cout << "Constraint: " << data.first << "\n";
                            for(std::size_t row : data.second) {
                                std::cout << row << " ";
                            }
                            std::cout << "\n";
                        }
                        */
                    }

                    std::vector<std::pair<std::vector<TYPE>, std::set<std::size_t>>> get_constraints() {
                        // joins constraints with identic selectors into a single gate

                        // drop the constraint_id from the stored id->(constraint,row_list) map:
                        std::vector<std::pair<std::vector<TYPE>, std::set<std::size_t>>> res;
                        for(const auto& [id, data] : *constraints) {
                            res.push_back({{data.first},data.second});
                        }
                        // join constrains into single element if they have the same row list:
                        for(std::size_t i = 0; i < res.size(); i++) {
                            for(std::size_t j = i + 1; j < res.size(); ) {
                                if (res[j].second == res[i].second) {
                                    res[i].first.insert(res[i].first.end(), res[j].first.begin(), res[j].first.end());
                                    res.erase(res.begin() + j);
                                } else {
                                    j++;
                                }
                            }
                        }
                        return res;
                    }

                    std::vector<plonk_copy_constraint>& get_copy_constraints() {
                        return *copy_constraints;
                    }

                    std::vector<std::pair<std::vector<lookup_constraint_type>, std::set<std::size_t>>> get_lookup_constraints() {
                        // std::map<std::pair<std::string,constraint_id_type>, // <table_name,expressions_id>
                        //          std::pair<std::vector<constraint_type>,std::set<std::size_t>>> // expressions, rows

                        std::vector<std::pair<std::vector<lookup_constraint_type>, std::set<std::size_t>>> res;
                        for(const auto& [id, data] : *lookup_constraints) {
                            res.push_back({{{id.first,data.first}}, data.second});
                        }
                        // join constrains into single element if they have the same row list:
                        for(std::size_t i = 0; i < res.size(); i++) {
                            for(std::size_t j = i + 1; j < res.size(); ) {
                                if (res[j].second == res[i].second) {
                                    res[i].first.insert(res[i].first.end(), res[j].first.begin(), res[j].first.end());
                                    res.erase(res.begin() + j);
                                } else {
                                    j++;
                                }
                            }
                        }
                        /*
                        for(const auto& [lcs, rows] : res) {
                            for(const auto& [table, cs] : lcs) {
                                std::cout << "Table " << table << ": ";
                                for(const auto& c : cs) { std::cout << c << ", "; }
                            }
                            std::cout << "Rows: ";
                            for(const auto& r : rows) { std::cout << r << " "; }
                            std::cout << "\n";
                        }
                        */
                        return res;
                    }

                    context subcontext(std::vector<std::size_t> W, std::size_t new_row_shift, std::size_t new_max_rows) {
                         context res = *this;
                         std::vector<std::size_t> new_W = {};
                         for(std::size_t i = 0; i < W.size(); i++) {
                             new_W.push_back(col_map[column_type::witness][W[i]]);
                         }
                         res.col_map[column_type::witness] = new_W;
                         res.row_shift += new_row_shift;
                         res.max_rows = new_max_rows;
                         res.current_row[column_type::witness] = 0; // reset to 0, because in the new column set everything is different
                         return res;
                    }

                    context(assignment_type &at, std::size_t max_rows) : basic_context<FieldType>(at, max_rows) {
                        constraints = std::make_shared<constraints_container_type>();
                        copy_constraints = std::make_shared<copy_constraints_container_type>();
                        lookup_constraints = std::make_shared<lookup_constraints_container_type>();
                    };

                    context(assignment_type &at, std::size_t max_rows, std::size_t row_shift) :
                        basic_context<FieldType>(at, max_rows, row_shift) {
                        constraints = std::make_shared<constraints_container_type>();
                        copy_constraints = std::make_shared<copy_constraints_container_type>();
                        lookup_constraints = std::make_shared<lookup_constraints_container_type>();
                    };

            };

            template<typename FieldType, GenerationStage stage>
            class generic_component {
                public:
                    using TYPE = typename std::conditional<static_cast<bool>(stage),
                                 crypto3::zk::snark::plonk_constraint<FieldType>,
                                 typename FieldType::value_type>::type;
                    using context_type = context<FieldType, stage>;
                    using plonk_copy_constraint = crypto3::zk::snark::plonk_copy_constraint<FieldType>;

                private:
                    context_type &ct;

                public:
                    void allocate(TYPE &C, column_type t = column_type::witness) {
                        auto [col, row] = ct.next_free_cell(t);
                        ct.allocate(C,col,row,t);
                    }

                    void allocate(TYPE &C, size_t col, size_t row, column_type t = column_type::witness) {
                        ct.allocate(C,col,row,t);
                    }

                    void copy_constrain(TYPE &A, TYPE &B) {
                        ct.copy_constrain(A,B);
                    }

                    void constrain(TYPE C) {
                        ct.constrain(C);
                    }

                    void lookup(std::vector<TYPE> &C, std::string table_name) {
                        ct.lookup(C,table_name);
                    }

                    void lookup(TYPE &C, std::string table_name) {
                        std::vector<TYPE> input = {C};
                        ct.lookup(input,table_name);
                    }

                    generic_component(context_type &context_object, // context object, created outside
                                      bool crlf = true              // do we assure a component starts on a new row? Default is "yes"
                                     ) : ct(context_object) {
                        if (crlf) { // TODO: Implement crlf parameter consequences
                            ct.new_line(column_type::witness);
                        }
                    };
            };

        } // namespace bbf
    } // namespace blueprint
} // namespace nil

#endif // CRYPTO3_BLUEPRINT_PLONK_BBF_GENERIC_HPP
