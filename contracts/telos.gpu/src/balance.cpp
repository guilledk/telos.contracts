#include <telos.gpu/telos.gpu.hpp>
#include <telos.gpu/utils.hpp>

namespace telos {

    void gpu::deposit(
        const name& from,
        const name& to,
        const asset& quantity,
        const string& memo
    ) {
        if (from == get_self() || to != get_self())
            return;

        check(global_config_instance.exists(), "gpu contract not configured yet");

        const auto& g_config = global_config_instance.get();

        check(get_first_receiver() == g_config.token_contract, "wrong token contract");

        check(quantity.amount > 0, "can only deposit non zero amount");
        check(quantity.symbol == g_config.token_symbol, "sent wrong token");

        add_balance(from, quantity, get_self());
    }

    void gpu::withdraw(const name& user, const asset& quantity) {
        require_auth(user);

        check(global_config_instance.exists(), "gpu contract not configured yet");

        const auto& g_config = global_config_instance.get();

        sub_balance(user, quantity);

        action(
            permission_level{get_self(),"active"_n},
            g_config.token_contract,
            "transfer"_n,
            make_tuple(get_self(), user, quantity, "withdraw: " + quantity.to_string())
        ).send();
    }

    void gpu::sub_balance(const name& owner, const asset& value) {
        users _users(get_self(), get_self().value);
        const auto& from =_users.find(owner.value);
        check(from != _users.end(), "no user account object found");
        check(from->balance.amount >= value.amount, "overdrawn balance");

        _users.modify(from, owner, [&]( auto& a ) {
            a.balance -= value;
        });
    }

    void gpu::add_balance(const name& owner, const asset& value, const name& ram_payer) {
        users _users(get_self(), get_self().value);
        auto to = _users.find(owner.value);
        if(to == _users.end()) {
            _users.emplace(ram_payer, [&](auto& a) {
                a.user = owner;
                a.balance = value;
            });
        } else {
            _users.modify(to, same_payer, [&](auto& a) {
                a.balance += value;
            });
        }
    }
}  // namespace
