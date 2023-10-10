#include <telos.gpu/telos.gpu.hpp>

namespace telos {

    void gpu::config(
        const name& token_contract,
        const symbol& token_symbol,
        const uint64_t& nonce
    ) {
        require_auth(get_self());

        auto g_config = global_config_instance.get_or_create(
            get_self(), global_config_row
        );

        g_config.token_contract = token_contract;
        g_config.token_symbol = token_symbol;
        g_config.nonce = nonce;

        global_config_instance.set(g_config, get_self());
    }

#ifdef TESTING
    void gpu::clean() {
        require_auth(get_self());
        work_queue _queue(get_self(), get_self().value);
        work_results _results(get_self(), get_self().value);

        auto qit = _queue.begin();
        while (qit != _queue.end())
            qit = _queue.erase(qit);

        auto rit = _results.begin();
        while (rit != _results.end())
            rit = _results.erase(rit);
    }
#endif
}  // namespace
