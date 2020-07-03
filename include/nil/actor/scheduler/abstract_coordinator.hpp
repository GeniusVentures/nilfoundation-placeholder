//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

#include <nil/actor/actor.hpp>
#include <nil/actor/actor_addr.hpp>
#include <nil/actor/actor_cast.hpp>
#include <nil/actor/actor_clock.hpp>
#include <nil/actor/defaults.hpp>
#include <nil/actor/spawner.hpp>

#include <nil/actor/fwd.hpp>
#include <nil/actor/message.hpp>

namespace nil {
    namespace actor {
        namespace scheduler {
            struct scheduler_config {
                string_view policy = defaults::scheduler::policy;
                string_view profiling_output_file = defaults::scheduler::profiling_output_file;
                size_t max_threads = defaults::scheduler::max_threads;
                size_t max_throughput = defaults::scheduler::max_throughput;
                timespan profiling_resolution = defaults::scheduler::profiling_resolution;
            };

            /// A coordinator creates the workers, manages delayed sends and
            /// the central printer instance for {@link aout}. It also forwards
            /// sends from detached workers or non-actor threads to randomly
            /// chosen workers.
            class BOOST_SYMBOL_VISIBLE abstract_coordinator
                : public spawner_module,
                  public module::configurable<scheduler_config, scheduler_config> {

                typedef module::configurable<scheduler_config, scheduler_config> policy_type;

            public:
                typedef typename policy_type::configuration_type configuration_type;
                typedef typename policy_type::options_types options_type;

                enum utility_actor_id : size_t { printer_id, max_id };

                explicit abstract_coordinator(spawner &sys);

                /// Returns a handle to the central printing actor.
                inline actor printer() const {
                    return actor_cast<actor>(utility_actors_[printer_id]);
                }

                /// Returns the number of utility actors.
                inline size_t num_utility_actors() const {
                    return utility_actors_.size();
                }

                /// Puts `what` into the queue of a randomly chosen worker.
                virtual void enqueue(resumable *what) = 0;

                inline spawner &system() {
                    return system_;
                }

                const spawner_config &config() const;

                inline size_t max_throughput() const {
                    return max_throughput_;
                }

                inline size_t num_workers() const {
                    return num_workers_;
                }

                /// Returns `true` if this scheduler detaches its utility actors.
                virtual bool detaches_utility_actors() const;

                void startup() override;

                void initialize(scheduler_config &cfg) override;

                virtual name_type name() const override {
                    return nullptr;
                }
                virtual void shutdown() override {
                }
                virtual void set_options(scheduler_config &cfg) const override {
                }
                virtual state_type state() const override {
                    return initialized;
                }
                virtual void handle_sighup() override {
                }

                id_t id() const override {
                    return 0x00;
                }

                void *subtype_ptr() override;

                static void cleanup_and_release(resumable *);

                virtual actor_clock &clock() noexcept = 0;

            protected:
                void stop_actors();

                /// ID of the worker receiving the next enqueue (round-robin dispatch).
                std::atomic<size_t> next_worker_;

                /// Number of messages each actor is allowed to consume per resume.
                size_t max_throughput_;

                /// Configured number of workers.
                size_t num_workers_;

                /// Background workers, e.g., printer.
                std::array<actor, max_id> utility_actors_;

                /// Reference to the host system.
                spawner &system_;
            };
        }    // namespace scheduler
    }        // namespace actor
}    // namespace nil