#pragma once

#include "token.hpp"

namespace lexer {
    struct Diagnostic {
        ErrorKind err_kind;
        std::size_t err_col{};
        bool had_err = false;

        constexpr void recordFirstError(ErrorKind new_err_kind, std::size_t col) noexcept {
            if (!had_err) {
                err_kind = new_err_kind;
                err_col = col;
                had_err = true;
            }
        }
    };
} // namespace lexer