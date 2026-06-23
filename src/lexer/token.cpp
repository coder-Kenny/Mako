#include <string_view>

#include "token.hpp"

namespace lexer {
    // -----------------------------------------------------------------------------
    //  String Conversions
    // -----------------------------------------------------------------------------
    std::string_view toString(TokenKind tok_kind) noexcept {
        switch (tok_kind) {
            case TokenKind::None: return "none"; // no token recognized

            #define PUNCTUATOR(name, str) case TokenKind::name: return #name;
            #define OPERATOR(name, str) case TokenKind::name: return #name;
            #define KEYWORD(name, str) case TokenKind::name: return #name;
            #define TOK_KIND(name) case TokenKind::name: return #name;
            #include "token_kinds.def"
            #undef TOK_KIND
            #undef KEYWORD
            #undef OPERATOR
            #undef PUNCTUATOR

            default: return "unknown token kind";
        }
    }

    std::string_view toString(ErrorKind err_kind) noexcept {
        switch (err_kind) {
            #define ERR_KIND(name, str) case ErrorKind::name: return str;
            #include "error_kinds.def"
            #undef ERR_KIND

            // fallback
            default: return "unknown error";
        }
    }

    std::string_view toString(const Range& span) noexcept {
        return span.view();
    }

    std::string toString(const Position& pos) {
        return std::to_string(pos.line) + ":" + std::to_string(pos.col);
    }

    std::string toString(const Token& token) {
        std::string out;

        // position
        out += toString(token.tok_pos);
        out += ": ";

        // kind
        out += std::string(toString(token.tok_kind));

        // lexeme
        if (!token.tok_span.empty()) {
            out += " '";
            out += std::string(token.tok_span.view());
            out += '\'';
        }

        return out;
    }

    std::string toString(const ErrorToken& err_token) {
        std::string out;

        // position
        out += toString(err_token.err_pos);
        out += ": ";

        // human-readable message
        out += err_token.msg;

        // lexeme
        if (!err_token.tok_span.empty()) {
            out += " '";
            out += std::string(err_token.tok_span.view());
            out += '\'';
        }

        return out;
    }
} // namespace lexer
