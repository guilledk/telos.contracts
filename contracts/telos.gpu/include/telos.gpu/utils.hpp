#pragma once

#include <telos.gpu/telos.gpu.hpp>

namespace telos {

    // small math helpers, only do inline here

    inline int64_t ipow(int64_t base, uint64_t exp) {
        if (exp == 0) return 1;

        int64_t result = base;
        for(int i = 0; i < (exp - 1); i++)
            result *= base;

        return result;
    }

    inline asset divide(const asset &A, const asset &B) {
        check(B.amount > 0, "can't divide by zero");
        check(A.symbol.precision() == B.symbol.precision(), "same precision only");

        int128_t _a = A.amount;
        int128_t _b = B.amount;

        // perform operation and add extra precision destroyed by operation
        int128_t result = (
            _a * ipow(10, B.symbol.precision())
        ) / _b;

        return asset((uint64_t)result, A.symbol);
    }

    inline asset multiply(const asset &A, const asset &B) {
        check(A.symbol.precision() == B.symbol.precision(), "same precision only");
        int128_t _a_amount = A.amount;
        int128_t _b_amount = B.amount;

        // perform operation and remove extra precision created by operation
        int128_t result = _a_amount * _b_amount;
        result /= ipow(10, A.symbol.precision());

        return asset((uint64_t)result, A.symbol);
    }

}
