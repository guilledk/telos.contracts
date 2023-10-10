#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <vector>
#include <string>

using namespace eosio;

#define TESTING


namespace telos {

    using std::string;
    using std::vector;
    using std::to_string;

    // contract

    class [[eosio::contract("telos.gpu")]] gpu : public contract {
        public:
            using contract::contract;

            gpu(name receiver, name code, datastream<const char*> ds) :
                contract(receiver, code, ds),
                global_config_instance(receiver, receiver.value) {}

            // system methods

            // initialize global config
            [[eosio::action]]
            void config(
                const name& token_contract,
                const symbol& token_symbol,
                const uint64_t& nonce
            );

#ifdef TESTING
            // clear all tables
            [[eosio::action]]
            void clean();
#endif

            // user methods

            // deposit handler
            [[eosio::on_notify("*::transfer")]]
            void deposit(
                const name& from,
                const name& to,
                const asset& quantity,
                const string& memo
            );

            [[eosio::action]]
            void withdraw(const name& user, const asset& quantity);

            // to push work to queue
            [[eosio::action]]
            void enqueue(
                const name& user,
                const string& request_body,
                const std::vector<string>& binary_inputs,
                const asset& reward,
                const uint32_t min_verification
            );

            // to cancel enqueued work
            [[eosio::action]]
            void dequeue(const name& user, const uint64_t request_id);

            // worker methods

            // inform about general worker info
            void regworker(
                const name& account,
                const string& url
            );

            // inform about worker compute power
            void addcard(
                const name& worker,
                const string& card_name,
                const string& version,
                uint64_t total_memory,
                uint32_t mp_count,
                const string& extra
            );

            // verify another worker's card proof
            // void verifycard(
            //     const name& worker,
            //     const name& other_worker,
            //     uint64_t card_id
            // );

            // remove a card when not online
            void remcard(
                const name& worker,
                uint64_t id
            );

            void unregworker(const name& account, const string& reason);

            // update worker status: begin
            [[eosio::action]]
            void workbegin(
                const name& worker, const uint64_t request_id, uint32_t max_workers);

            // update worker status: generic
            [[eosio::action]]
            void workupdate(
                const name& worker, const uint64_t request_id, const string& status);

            // update worker status: cancelled
            [[eosio::action]]
            void workcancel(
                const name& worker, const uint64_t request_id, const string& reason);

            // after doing work, if we get min_verification matches clear out request
            [[eosio::action]]
            void submit(
                const name& worker,
                const uint64_t request_id,
                const checksum256& request_hash,
                const checksum256& result_hash,
                const string& ipfs_hash
            );

        private:
            struct [[eosio::table]] global_configuration_struct {
                name token_contract;
                symbol token_symbol;
                uint64_t nonce;
            } global_config_row;

            typedef eosio::singleton<"config"_n, global_configuration_struct> global_config;
            global_config global_config_instance;

            struct [[eosio::table]] account {
                name user;
                asset    balance;

                uint64_t primary_key()const { return user.value; }
            };
            typedef eosio::multi_index<"users"_n, account> users;

            struct [[eosio::table]] worker {
                name account;
                time_point_sec joined;
                time_point_sec left;
                string url;

                uint64_t primary_key()const { return account.value; }
            };
            typedef eosio::multi_index<"workers"_n, worker> workers;

            struct [[eosio::table]] card {
                uint64_t id;

                name owner;
                string card_name;
                string version;
                uint64_t total_memory;
                uint32_t mp_count;
                string extra;

                // string ipfs_proof;
                // uint32_t verifications;
                // time_point_sec last_verified;

                uint64_t primary_key() const { return id; }
                uint64_t by_owner() const { return owner.value; }
            };
            typedef eosio::multi_index<
                "cards"_n,
                card,
                indexed_by<
                    "byowner"_n, const_mem_fun<card, uint64_t, &card::by_owner>
                >
            > cards;

            struct worker_status_struct {
                name worker;
                string status;
                time_point_sec started;
            };

            struct [[eosio::table]] work_request_struct {
                uint64_t nonce;

                name user;
                asset reward;
                uint32_t min_verification;

                string body;
                std::vector<string> binary_inputs;

                std::vector<worker_status_struct> status;

                time_point_sec timestamp;

                uint64_t primary_key() const { return nonce; }
                uint64_t by_time() const { return (uint64_t)timestamp.sec_since_epoch(); }
            };

            typedef eosio::multi_index<
                "queue"_n,
                work_request_struct,
                indexed_by<
                    "bytime"_n, const_mem_fun<work_request_struct, uint64_t, &work_request_struct::by_time>
                >
            > work_queue;

            struct [[eosio::table]] work_result_struct {
                uint64_t id;
                uint64_t request_id;
                name user;
                name worker;
                checksum256 result_hash;
                string ipfs_hash;
                time_point_sec submited;

                uint64_t primary_key() const { return id; }
                uint64_t by_request_id() const { return request_id; }
                checksum256 by_result_hash() const { return result_hash; }
                uint64_t by_worker() const { return worker.value; }
                uint64_t by_time() const { return (uint64_t)submited.sec_since_epoch(); }
            };

            typedef eosio::multi_index<
                "results"_n,
                work_result_struct,
                indexed_by<
                    "byreqid"_n, const_mem_fun<work_result_struct, uint64_t, &work_result_struct::by_request_id>
                >,
                indexed_by<
                    "byresult"_n, const_mem_fun<work_result_struct, checksum256, &work_result_struct::by_result_hash>
                >,
                indexed_by<
                    "byworker"_n, const_mem_fun<work_result_struct, uint64_t, &work_result_struct::by_worker>
                >,
                indexed_by<
                    "bytime"_n, const_mem_fun<work_result_struct, uint64_t, &work_result_struct::by_time>
                >
            > work_results;

            // helpers
            uint64_t get_and_increment_nonce() {
                auto g_config = global_config_instance.get();
                uint64_t nonce = g_config.nonce;
                g_config.nonce += 1;
                global_config_instance.set(g_config, get_self());
                return nonce;
            }

            void sub_balance(const name& owner, const asset& value);
            void add_balance(const name& owner, const asset& value, const name& ram_payer);
   };

}
