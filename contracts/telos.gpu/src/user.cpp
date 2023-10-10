#include <telos.gpu/telos.gpu.hpp>

namespace telos {

    void gpu::enqueue(
        const name& user,
        const string& request_body,
        const std::vector<string>& binary_inputs,
        const asset& reward,
        const uint32_t min_verification
    ) {
        require_auth(user);

        // escrow the funds during the request life
        sub_balance(user, reward);

        uint64_t nonce = get_and_increment_nonce();

        work_queue _queue(get_self(), get_self().value);
        _queue.emplace(user, [&](auto& r) {
            r.nonce = nonce;
            r.user = user;
            r.reward = reward;
            r.min_verification = min_verification;
            r.body = request_body;
            r.binary_inputs = binary_inputs;
            r.timestamp = current_time_point();

            print(to_string(nonce));
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
}  // namespace
