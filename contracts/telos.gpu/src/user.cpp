#include <telos.gpu/telos.gpu.hpp>

namespace telos {

    void gpu::enqueue(
        const name& user,
        const string& request_body,
        const string& binary_data,
        const asset& reward,
        const uint32_t min_verification
    ) {
        require_auth(user);

        // escrow the funds during the request life
        sub_balance(user, reward);

        uint64_t prev_nonce = increment_nonce(user);

        work_queue _queue(get_self(), get_self().value);
        _queue.emplace(user, [&](auto& r) {
            r.id = _queue.available_primary_key();
            r.user = user;
            r.reward = reward;
            r.min_verification = min_verification;
            r.nonce = prev_nonce;
            r.body = request_body;
            r.binary_data = binary_data;
            r.timestamp = current_time_point();

            print(to_string(r.id) + ":" + to_string(prev_nonce));
        });
    }

    void gpu::dequeue(const name& user, const uint64_t request_id) {
        require_auth(user);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        // release reward escrow
        add_balance(user, rit->reward, user);

        _queue.erase(rit);
    }

    uint64_t gpu::increment_nonce(const name& owner) {
        users _users(get_self(), get_self().value);
        const auto& it =_users.find(owner.value);
        check(it != _users.end(), "no user account object found");

        uint64_t prev_nonce;
        _users.modify(it, owner, [&](auto& a) {
            prev_nonce = a.nonce++;
        });
        return prev_nonce;
    }
}  // namespace
