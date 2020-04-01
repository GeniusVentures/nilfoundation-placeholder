//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE intrusive_ptr

#include <nil/actor/intrusive_ptr.hpp>

#include <nil/actor/test/dsl.hpp>

// this test doesn't verify thread-safety of intrusive_ptr
// however, it is thread safe since it uses atomic operations only

#include <vector>
#include <cstddef>

#include <nil/actor/ref_counted.hpp>
#include <nil/actor/make_counted.hpp>

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<template<typename...> class T, typename... P>
            struct print_log_value<T<P...>> {
                void operator()(std::ostream &, T<P...> const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

using namespace nil::actor;

namespace {

    int class0_instances = 0;
    int class1_instances = 0;

    class class0;
    class class1;

    using class0ptr = intrusive_ptr<class0>;
    using class1ptr = intrusive_ptr<class1>;

    class class0 : public ref_counted {
    public:
        explicit class0(bool subtype = false) : subtype_(subtype) {
            if (!subtype) {
                ++class0_instances;
            }
        }

        ~class0() override {
            if (!subtype_) {
                --class0_instances;
            }
        }

        bool is_subtype() const {
            return subtype_;
        }

        virtual class0ptr create() const {
            return make_counted<class0>();
        }

    private:
        bool subtype_;
    };

    class class1 : public class0 {
    public:
        class1() : class0(true) {
            ++class1_instances;
        }

        ~class1() override {
            --class1_instances;
        }

        class0ptr create() const override {
            return make_counted<class1>();
        }
    };

    class0ptr get_test_rc() {
        return make_counted<class0>();
    }

    class0ptr get_test_ptr() {
        return get_test_rc();
    }

    struct fixture {
        ~fixture() {
            BOOST_CHECK_EQUAL(class0_instances, 0);
            BOOST_CHECK_EQUAL(class1_instances, 0);
        }
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(atom_tests, fixture)

BOOST_AUTO_TEST_CASE(make_counted_test) {
    auto p = make_counted<class0>();
    BOOST_CHECK_EQUAL(class0_instances, 1);
    BOOST_CHECK(p->unique());
}

BOOST_AUTO_TEST_CASE(reset_test) {
    class0ptr p;
    p.reset(new class0, false);
    BOOST_CHECK_EQUAL(class0_instances, 1);
    BOOST_CHECK(p->unique());
}

BOOST_AUTO_TEST_CASE(get_test_rc_test) {
    class0ptr p1;
    p1 = get_test_rc();
    class0ptr p2 = p1;
    BOOST_CHECK_EQUAL(class0_instances, 1);
    BOOST_CHECK_EQUAL(p1->unique(), false);
}

BOOST_AUTO_TEST_CASE(list) {
    std::vector<class0ptr> pl;
    pl.push_back(get_test_ptr());
    pl.push_back(get_test_rc());
    pl.push_back(pl.front()->create());
    BOOST_CHECK(pl.front()->unique());
    BOOST_CHECK_EQUAL(class0_instances, 3);
}

BOOST_AUTO_TEST_CASE(full_test) {
    auto p1 = make_counted<class0>();
    BOOST_CHECK_EQUAL(p1->is_subtype(), false);
    BOOST_CHECK_EQUAL(p1->unique(), true);
    BOOST_CHECK_EQUAL(class0_instances, 1);
    BOOST_CHECK_EQUAL(class1_instances, 0);
    p1.reset(new class1, false);
    BOOST_CHECK_EQUAL(p1->is_subtype(), true);
    BOOST_CHECK_EQUAL(p1->unique(), true);
    BOOST_CHECK_EQUAL(class0_instances, 0);
    BOOST_CHECK_EQUAL(class1_instances, 1);
    auto p2 = make_counted<class1>();
    p1 = p2;
    BOOST_CHECK_EQUAL(p1->unique(), false);
    BOOST_CHECK_EQUAL(class0_instances, 0);
    BOOST_CHECK_EQUAL(class1_instances, 1);
    BOOST_CHECK_EQUAL(p1, static_cast<class0 *>(p2.get()));
}

BOOST_AUTO_TEST_SUITE_END()
