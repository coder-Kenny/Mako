#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace lexer {
    // ==== Token Kinds via X-macro ====
    enum class TokenKind : uint8_t {
        None = 0, // no token recognized

        #define PUNCTUATOR(name, str) name,
        #define OPERATOR(name, str) name,
        #define KEYWORD(name, str) name,
        #define TOK_KIND(name) name,
        #include "token_kinds.def"
        #undef TOK_KIND
        #undef KEYWORD
        #undef OPERATOR
        #undef PUNCTUATOR
    };

    // ==== Error Kinds via X-macro ====
    enum class ErrorKind : uint8_t {
        #define ERR_KIND(name, str) name,
        #include "error_kinds.def"
        #undef ERR_KIND
    };

    // ==== Half-open Character Range [begin, end) ====
    struct Range {
        const char* begin{}; // pointer to the first character
        const char* end{}; // pointer one past the last character

        [[nodiscard]] constexpr std::string_view view() const noexcept {
            if (empty()) return {};
            return {begin, size()};
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(end - begin); }
        [[nodiscard]] constexpr bool empty() const noexcept { return begin == end; }
    };

    // ==== 1-based Source Position ====
    struct Position {
        std::size_t line{}; // 1-based
        std::size_t col{}; // 1-based
    };

    // ==== Lexer Output Tokens ====
    struct Token {
        TokenKind tok_kind;
        Range tok_span;
        Position tok_pos;

        [[nodiscard]] constexpr std::string_view lexeme() const noexcept { return tok_span.view(); }
    };

    // ==== Lexer Error Output Tokens (separate from token stream) ====
    struct ErrorToken {
        ErrorKind err_kind;
        Range tok_span;
        Position err_pos;
        std::string_view msg{}; // human-readable error message

        [[nodiscard]] constexpr std::string_view lexeme() const noexcept { return tok_span.view(); }
    };

    // ==== String Conversions ====
    [[nodiscard]] std::string_view toString(TokenKind tok_kind) noexcept;
    [[nodiscard]] std::string_view toString(ErrorKind err_kind) noexcept;
    [[nodiscard]] std::string_view toString(const Range& span) noexcept;
    [[nodiscard]] std::string toString(const Position& pos);
    [[nodiscard]] std::string toString(const Token& token);
    [[nodiscard]] std::string toString(const ErrorToken& err_token);

    // ==== Error Message Mapping ====
    [[nodiscard]] constexpr std::string_view errorMessage(ErrorKind err_kind) noexcept {
        return toString(err_kind);
    }
} // namespace lexer
