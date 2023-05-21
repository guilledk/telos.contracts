#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

#include <string>

using namespace eosio;

namespace telos {

    using std::string;

    class [[eosio::contract("telos.gpu")]] gpu : public contract {
        public:
            using contract::contract;

            // user methods

            // to push work to queue
            [[eosio::action]]
            void enqueue(const name& user, const string& request_data);

            // to cancel enqueued work
            [[eosio::action]]
            void dequeue(const name& user, const uint64_t request_id);

            // worker methods

            // update worker status: begin
            [[eosio::action]]
            void workbegin(const name& worker, const uint64_t request_id);

            // update worker status: cancelled
            [[eosio::action]]
            void workcancel(const name& worker, const uint64_t request_id);

            // after doing work, if we get three hash matches clear out request
            [[eosio::action]]
            void submit(
                const name& worker,
                const uint64_t request_id,
                const checksum256 result_hash
            );

        private:
            struct [[eosio::table]] work_request_struct {
                uint64_t id;
                name user;
                string data;
                time_point_sec timestamp;

                uint64_t primary_key() const { return id; }
                uint64_t by_time() const { return (uint64_t)timestamp.sec_since_epoch(); }
            };

            struct [[eosio::table]] worker_status_struct {
                // SCOPED TO request_id
                name worker;
                string status;
                time_point_sec started;

                uint64_t primary_key() const { return worker.value; }
            };

            struct [[eosio::table]] work_result_struct {
                uint64_t id;
                uint64_t request_id;
                name user;
                name worker;
                checksum256 result_hash;
                time_point_sec submited;

                uint64_t primary_key() const { return id; }
                checksum256 by_result_hash() const { return result_hash; }
                uint64_t by_worker() const { return worker.value; }
            };

            typedef eosio::multi_index<
                "queue"_n,
                work_request_struct,
                indexed_by<
                    "bytime"_n, const_mem_fun<work_request_struct, uint64_t, &work_request_struct::by_time>
                >
            > work_queue;

            typedef eosio::multi_index<"status"_n, worker_status_struct> worker_status;

            typedef eosio::multi_index<
                "results"_n,
                work_result_struct,
                indexed_by<
                    "byresult"_n, const_mem_fun<work_result_struct, checksum256, &work_result_struct::by_result_hash>
                >,
                indexed_by<
                    "byworker"_n, const_mem_fun<work_result_struct, uint64_t, &work_result_struct::by_worker>
                >
            > work_results;
   };

}
