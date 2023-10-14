#include <telos.gpu/telos.gpu.hpp>
#include <telos.gpu/utils.hpp>

namespace telos {

    void gpu::regworker(
        const name& account,
        const string& url
    ) {
        require_auth(account);

        workers _workers(get_self(), get_self().value);

        auto it = _workers.find(account.value);

        if (it == _workers.end())
            _workers.emplace(account, [&](auto& w) {
                w.account = account;
                w.url = url;
                w.joined = current_time_point();
            });
        else
            _workers.modify(it, account, [&](auto& w) {
                w.url = url;
            });
    }

    void gpu::addcard(
        const name& worker,
        const string& card_name,
        const string& version,
        uint64_t total_memory,
        uint32_t mp_count,
        const string& extra
    ) {
        require_auth(worker);

        workers _workers(get_self(), get_self().value);

        auto it = _workers.find(worker.value);
        check(it != _workers.end(), "worker not registered");

        cards _cards(get_self(), get_self().value);
        _cards.emplace(worker, [&](auto& c) {
            c.id = _cards.available_primary_key();
            c.owner = worker;
            c.card_name = card_name;
            c.version = version;
            c.total_memory = total_memory;
            c.mp_count = mp_count;
            c.extra = extra;
        });
    }

    // void gpu::verifycard(
    //     const name& worker,
    //     const name& other_worker,
    //     uint64_t card_id
    // );

    void gpu::remcard(
        const name& worker,
        uint64_t id
    ) {
        require_auth(worker);

        cards _cards(get_self(), get_self().value);
        auto it = _cards.find(id);
        check(it != _cards.end(), "card not found");

        _cards.erase(it);
    }

    void gpu::unregworker(const name& account, const string& unreg_reason) {
        require_auth(account);

        workers _workers(get_self(), get_self().value);
        auto it = _workers.find(account.value);
        check(it != _workers.end(), "worker not registered");

        // clear out cards
        cards _cards(get_self(), get_self().value);
        auto worker_card_index = _cards.get_index<"byowner"_n>();
        auto worker_cards_it = worker_card_index.find(account.value);
        while (worker_cards_it != worker_card_index.end())
            worker_cards_it = worker_card_index.erase(worker_cards_it);

        _workers.erase(it);
    }

    void gpu::workbegin(const name& worker, const uint64_t request_id, uint32_t max_workers) {
        require_auth(worker);

        auto it = _workers.find(worker.value);
        check(it != _workers.end(), "worker not registered");

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        _queue.modify(rit, worker, [&](auto& r) {
            auto sit = std::find_if(r.status.begin(), r.status.end(),
                [&worker](const worker_status_struct item) {
                    return item.worker == worker;
                }
            );
            check(sit == r.status.end(), "request already started");
            check(r.status.size() <= max_workers, "too many workers already on this request");
            r.status.push_back({worker, "started", current_time_point()});
        });
    }

    void gpu::workupdate(const name& worker, const uint64_t request_id, const string& status) {
        require_auth(worker);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        _queue.modify(rit, worker, [&](auto& r) {
            auto sit = std::find_if(r.status.begin(), r.status.end(),
                [&worker](const worker_status_struct item) {
                    return item.worker == worker;
                }
            );
            check(sit != r.status.end(), "status not found");
            sit->status = status;
        });
    }

    void gpu::workcancel(
        const name& worker, const uint64_t request_id, const string& reason) {
        require_auth(worker);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        _queue.modify(rit, worker, [&](auto& r) {
            auto sit = std::find_if(r.status.begin(), r.status.end(),
                [&worker](const worker_status_struct item) {
                    return item.worker == worker;
                }
            );
            check(sit != r.status.end(), "status not found");
            r.status.erase(sit);
        });
    }

    void gpu::submit(
        const name& worker,
        const uint64_t request_id,
        const checksum256& request_hash,
        const checksum256& result_hash,
        const string& ipfs_hash
    ) {
        check(global_config_instance.exists(), "gpu contract not configured yet");
        require_auth(worker);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        string binary_inputs = "";
        for (const string& binary_input : rit->binary_inputs)
            binary_inputs += binary_input;

        const string& hash_str = to_string(rit->nonce) + rit->body + binary_inputs;
        print("hashing: " + hash_str + "\n");
        assert_sha256(hash_str.c_str(), hash_str.size(), request_hash);

        _queue.modify(rit, worker, [&](auto& r) {
            auto sit = std::find_if(r.status.begin(), r.status.end(),
                [&worker](const worker_status_struct item) {
                    return item.worker == worker;
                }
            );
            check(sit != r.status.end(), "status not found");
            r.status.erase(sit);
        });

        work_results _results(get_self(), get_self().value);
        auto worker_index = _results.get_index<"byworker"_n>();
        auto wit = worker_index.find(worker.value);
        check(wit == worker_index.end(), "already submited result");

        _results.emplace(worker, [&](auto& r) {
            r.id = _results.available_primary_key();
            r.request_id = request_id;
            r.user = rit->user;
            r.worker = worker;
            r.result_hash = result_hash;
            r.ipfs_hash = ipfs_hash;
            r.submited = current_time_point();
        });

        auto result_index = _results.get_index<"byresult"_n>();
        auto result_it = result_index.find(result_hash);

        uint32_t match = 0;
        while(result_it != result_index.end()) {
            result_it++;
            match++;
        }


        if (match == rit->min_verification) {
            // got enough matches, split reward between miners,
            // clear request results & queue and return

            vector<name> payments;

            // iterate over results by ascending timestamp
            // delete all related to this request but store
            // fist n miners (n == verification_amount)
            auto res_req_index = _results.get_index<"bytime"_n>();
            auto _clear_it = res_req_index.lower_bound(
                (uint64_t)rit->timestamp.sec_since_epoch());

            while(_clear_it != res_req_index.end()) {
                if (_clear_it->request_id == request_id) {
                    if (payments.size() < rit->min_verification - 1 &&
                        _clear_it->worker != worker)
                        payments.push_back(_clear_it->worker);

                    _clear_it = res_req_index.erase(_clear_it);
                } else
                    _clear_it++;
            }

            payments.push_back(worker);

            print("paying:\n");
            for (const auto& miner : payments)
                print(miner.to_string() + "\n");

            // split reward and add it to miner balances
            const auto& g_config = global_config_instance.get();
            asset split_factor = divide(
                asset(1, g_config.token_symbol),  // 1 unit
                asset(rit->min_verification, g_config.token_symbol)
            );
            print("reward: " + rit->reward.to_string() + "\n");
            print("factor: " + split_factor.to_string() + "\n");

            asset payment = multiply(rit->reward, split_factor);

            print("payment: " + payment.to_string() + "\n");

            for(const auto& miner : payments)
                add_balance(miner, payment, miner);

            require_recipient(rit->user);

            _queue.erase(rit);
            return;
        }
    }
}  // namespace
