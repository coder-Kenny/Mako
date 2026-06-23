#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "diagnostic.hpp"
#include "token.hpp"

namespace lexer {
    // -----------------------------------------------------------------------------
    //  Data Structures
    // -----------------------------------------------------------------------------
    struct BufferCursor {
        const char* cur{};
        const char* end{};
    };

    struct LexerState {
        BufferCursor buffer;
        Position pos; // position = line & column, see token.hpp
    };

    // -----------------------------------------------------------------------------
    //  Lookup Tables
    // -----------------------------------------------------------------------------
    enum class CharKind : uint16_t {
        Illegal    = 0,
        Space      = 1 << 0,
        NewLine    = 1 << 1,
        Escape     = 1 << 3,
        Punctuator = 1 << 4,
        Operator   = 1 << 5,
        Alpha      = 1 << 6,
        Underscore = 1 << 7,
        Digit      = 1 << 8,
        DecPoint   = 1 << 9,
        Binary     = 1 << 10,
        Hex        = 1 << 11,
        Octal      = 1 << 12,
        Quote      = 1 << 13,
        Eof        = 1 << 14
    };

    // ==== Chartable ====
    [[nodiscard]] constexpr auto makeCharTable() noexcept {
        std::array<std::uint16_t, 256> t{};

        auto setFlag = [&](char c, CharKind flag) {
            auto uc = static_cast<unsigned char>(c);
            t[uc] |= static_cast<std::uint16_t>(flag);
        };

        // space, newline & escape
        for (char c : {' ', '\t', '\n', '\r', '\f', '\v'}) setFlag(c, CharKind::Space);
        for (char c : {'\n', '\r', '\f'}) setFlag(c, CharKind::NewLine);
        for (char c : {'t', 'n', 'r', 'f', 'v', '0', '\\', '\'', '"'}) setFlag(c, CharKind::Escape);

        // punctuators & operators
        #define PUNCTUATOR(name, str) setFlag(str[0], CharKind::Punctuator);
        // the first operator character encodes all valid operator characters.
        #define OPERATOR(name, str) setFlag(str[0], CharKind::Operator);
        #include "token_kinds.def"
        #undef OPERATOR
        #undef PUNCTUATOR

        // alpha
        for (char c = 'a'; c <= 'z'; ++c) setFlag(c, CharKind::Alpha);
        for (char c = 'A'; c <= 'Z'; ++c) setFlag(c, CharKind::Alpha);

        // underscore
        setFlag('_', CharKind::Underscore);

        // numbers
        for (char c = '0'; c <= '9'; ++c) setFlag(c, CharKind::Digit);
        setFlag('.', CharKind::DecPoint);


        setFlag('0', CharKind::Binary);
        setFlag('1', CharKind::Binary);

        for (char c = '0'; c <= '9'; ++c) setFlag(c, CharKind::Hex);
        for (char c = 'a'; c <= 'f'; ++c) setFlag(c, CharKind::Hex);
        for (char c = 'A'; c <= 'F'; ++c) setFlag(c, CharKind::Hex);

        for (char c = '0'; c <= '7'; ++c) setFlag(c, CharKind::Octal);

        // quotes
        setFlag('\'', CharKind::Quote);
        setFlag('\"', CharKind::Quote);

        // eof
        setFlag('\0', CharKind::Eof);

        return t;
    }

    inline constexpr auto charTable = makeCharTable();

    // ==== Illegal Characters ====
    [[nodiscard]] constexpr bool isIllegal(unsigned char uc) noexcept {
        // All non-ASCII bytes (>= 128) are illegal by design.
        // This language uses ASCII-only identifiers and tokens.
        return charTable[uc] == 0;
    }

    // ==== Whitespace, NewLine & Escape ====
    [[nodiscard]] constexpr bool isWhiteSpace(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Space);
    }
    [[nodiscard]] constexpr bool isNewLine(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::NewLine);
    }
    [[nodiscard]] constexpr bool isEscape(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Escape);
    }

    // ==== Punctuators ====
    [[nodiscard]] constexpr auto makePunctTable() noexcept {
        std::array<TokenKind, 256> t{};

        #define PUNCTUATOR(name, str) t[str[0]] = TokenKind::name;
        #include "token_kinds.def"
        #undef PUNCTUATOR

        return t;
    }

    inline constexpr auto isPunctuator = makePunctTable();

    // ==== Operators ====
    [[nodiscard]] constexpr bool isOperatorBegin(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Operator);
    }

    // ==== Alpha ====
    [[nodiscard]] constexpr bool isAlpha(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Alpha);
    }

    // ==== Identifiers ====
    [[nodiscard]] constexpr bool isIdBegin(unsigned char uc) noexcept {
        constexpr auto mask = static_cast<std::uint16_t>(CharKind::Alpha) |
                                 static_cast<std::uint16_t>(CharKind::Underscore);

        return charTable[uc] & mask;
    }

    [[nodiscard]] constexpr bool isIdPart(unsigned char uc) noexcept {
        constexpr auto mask = static_cast<std::uint16_t>(CharKind::Alpha) |
                                 static_cast<std::uint16_t>(CharKind::Digit) |
                                 static_cast<std::uint16_t>(CharKind::Underscore);

        return charTable[uc] & mask;
    }

    // ==== Numbers ====
    [[nodiscard]] constexpr bool isDigit(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Digit);
    }

    [[nodiscard]] constexpr bool isBinary(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Binary);
    }

    [[nodiscard]] constexpr bool isHex(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Hex);
    }

    [[nodiscard]] constexpr bool isOctal(unsigned char uc) noexcept {
        return charTable[uc] & static_cast<std::uint16_t>(CharKind::Octal);
    }

    [[nodiscard]] constexpr bool isNumberTerminator(unsigned char uc) noexcept {
        constexpr auto mask = static_cast<std::uint16_t>(CharKind::Digit) |
                              static_cast<std::uint16_t>(CharKind::Alpha) |
                              static_cast<std::uint16_t>(CharKind::DecPoint) |
                               static_cast<std::uint16_t>(CharKind::Underscore);

        return !(charTable[uc] & mask);
    }

    // ==== Keyword Lexing Utils ====
    [[nodiscard]] constexpr auto sorted_keywords() noexcept {
        std::pair<std::string_view, TokenKind> kws[] {
            #define KEYWORD(name, str) {str, TokenKind::name},
            #include "token_kinds.def"
            #undef KEYWORD
        };

        auto sorted = std::to_array(kws);

        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        return sorted;
    }

    inline constexpr auto keywords = sorted_keywords();

    // -----------------------------------------------------------------------------
    //  Lexer Class
    // -----------------------------------------------------------------------------
    class Lexer {
    public:
        Lexer(const char* begin, const char* end);

        Token nextToken() noexcept;

        std::vector<ErrorToken> errors;

    private:
        LexerState state_;

        // ==== Conversion (char -> unsigned char) ====
        [[nodiscard]] constexpr unsigned char to_uc(char c) const noexcept { return static_cast<unsigned char>(c); }

        // -----------------------------------------------------------------------------
        //  Cursor Navigation & Peeking
        // -----------------------------------------------------------------------------
        // WARNING: Increments columns only. Use skipNewLine() for newlines to keep position data consistent.
        char consume() noexcept {
            char c = *state_.buffer.cur++;
            ++state_.pos.col;
            return c;
        }

        [[nodiscard]] char peek(std::size_t offset = 0) const noexcept { return state_.buffer.cur[offset]; }

        // WARNING: Advances the cursor and columns blindy. Use skipNewLine() for newlines to keep it consistent.
        void advance(std::size_t n = 1) noexcept {
            state_.buffer.cur += n;
            state_.pos.col += n;
        }

        [[nodiscard]] bool match(char expected) noexcept {
            if (peek() == expected) {
                consume();
                return true;
            }

            return false;
        }

        // ==== Token Construction ====
        [[nodiscard]] constexpr Token makeToken(TokenKind tok_kind, const char* start, std::size_t col) const noexcept {
            return {tok_kind, {start, state_.buffer.cur}, {state_.pos.line, col}};
        }

        // ==== Error Token Construction ====
        [[nodiscard]] constexpr ErrorToken makeErrorToken(ErrorKind err_kind, const char* start, std::size_t col) const noexcept {
            return {err_kind,
           {start, state_.buffer.cur},
            {state_.pos.line, col},
                {errorMessage(err_kind)}
            };
        }

        // ==== Error Handling ====
        Token reportError(ErrorKind err_kind, const char* start, std::size_t col) noexcept {
            errors.push_back(makeErrorToken(err_kind, start, col));
            return makeToken(TokenKind::Error, start, col);
        }

        Token lexIllegalChars() noexcept;

        // ==== Skipping Utilities (whitespace & comments) ====
        void skipNewLine() noexcept;
        void skipWhiteSpace() noexcept;
        void skipLineComment() noexcept;
        // returns true on success, false on error (+ error token).
        bool skipBlockComment(const char* start, std::size_t col) noexcept;

        // ==== Operator Lexing ====
        Token lexOperator() noexcept;

        // ==== Keyword and Identifier Lexing ====
        [[nodiscard]] TokenKind getKeywordKind(std::string_view s) const noexcept {

            auto it = std::lower_bound(keywords.begin(), keywords.end(), s,
                [](const auto& kw, auto val) {
                    return kw.first < val;
            });

            if (it != keywords.end() && it->first == s) {
                return it->second;
            }
            return TokenKind::Identifier;
        }

        Token lexIdentifier() noexcept;

        // -----------------------------------------------------------------------------
        //  Number Lexing
        // -----------------------------------------------------------------------------
        void handleUnderscore(Diagnostic& diag) noexcept {
            unsigned char next = to_uc(peek(1));
            if (isNumberTerminator(next) || next == '_' || next == '.')
                diag.recordFirstError(ErrorKind::InvalidDigitSeparator, state_.pos.col + 1);
        }

        void consumeExponent(bool& had_exponent, Diagnostic& diag) noexcept;
        Token lexDecimal(const char* start, std::size_t col) noexcept;

        template <typename Predicate>
        void consumeDigitsByPredicate(Predicate isDigitFunc, Predicate isDelimiter, Diagnostic& diag) noexcept {
            while (true) {
                unsigned char uc = to_uc(peek());

                if (isDelimiter(uc)) break;

                if (isDigitFunc(uc)) {
                    consume();
                    continue;
                }

                if (uc == '_') {
                    handleUnderscore(diag);
                    consume();
                    continue;
                }

                diag.recordFirstError(ErrorKind::InvalidNumericLiteral, state_.pos.col);
                consume();
            }
        }

        template <typename Predicate>
        Token lexRadixLiteral(TokenKind tok_kind, Predicate isDigitFunc, Predicate isDelimiter, const char* start, std::size_t col) noexcept {
            Diagnostic diag;

            advance(2); // skip the radix marker (e.g. '0x')

            unsigned char uc = to_uc(peek());
            if (isDelimiter(uc)) return reportError(ErrorKind::MissingDigit, start, state_.pos.col);

            consumeDigitsByPredicate(isDigitFunc, isNumberTerminator, diag);

            if (diag.had_err) return reportError(diag.err_kind, start, diag.err_col);
            return makeToken(tok_kind, start, col);
        }

        [[nodiscard]] Token lexBinary(const char* start, std::size_t col) noexcept {
            return lexRadixLiteral(TokenKind::BinaryLiteral, isBinary, isNumberTerminator, start, col);
        }

        [[nodiscard]] Token lexHex(const char* start, std::size_t col) noexcept {
            return lexRadixLiteral(TokenKind::HexLiteral, isHex, isNumberTerminator, start, col);
        }

        [[nodiscard]] Token lexOctal(const char* start, std::size_t col) noexcept {
            return lexRadixLiteral(TokenKind::OctalLiteral, isOctal, isNumberTerminator, start, col);
        }

        Token lexNumber() noexcept;

        // -----------------------------------------------------------------
        // Char Lexing
        // -----------------------------------------------------------------
        Token lexChar() noexcept;

        // -----------------------------------------------------------------
        // String Lexing
        // -----------------------------------------------------------------
        void matchAndConsumeEscape(Diagnostic& diag) {
            unsigned char next = to_uc(peek(1));

            if (isEscape(next)) advance(2);
            else if (next == 'x' || next == 'X') consumeHexEscape(diag);
            else if (next == 'o' || next == 'O') consumeOctalEscape(diag);
            else if (next == 'u' || next == 'U') consumeUnicodeEscape(diag);
            else {
                diag.recordFirstError(ErrorKind::UnrecognizedEscape, state_.pos.col);
                consume(); // consume '\'
            }
        }

        template <typename Predicate>
        void consumeEscape(Predicate isDigitFunc, Diagnostic& diag) noexcept {
            advance(2); // skip the radix marker (e.g. '\x')

            if (isNumberTerminator(to_uc(peek())))
                diag.recordFirstError(ErrorKind::MissingDigit, state_.pos.col);

            consumeDigitsByPredicate(isDigitFunc, isDigitFunc, diag);
        }

        void consumeHexEscape(Diagnostic& diag) noexcept {
            consumeEscape(isHex, diag);
        }

        void consumeOctalEscape(Diagnostic& diag) noexcept {
            consumeEscape(isOctal, diag);
        }

        void consumeUnicodeEscape(Diagnostic& diag) noexcept {
            int fixed_length = 0;
            if (peek(1) == 'u') fixed_length = 4;
            else fixed_length = 8;

            advance(2); // skip the unicode marker ('\u' or '\U')

            if (isNumberTerminator(to_uc(peek())))
                diag.recordFirstError(ErrorKind::MissingDigit, state_.pos.col);

            for (int i = 0; i < fixed_length; ++i) {
                if (!isHex(to_uc(peek()))) {
                    diag.recordFirstError(ErrorKind::InvalidNumericLiteral, state_.pos.col);
                    if (!isHex(to_uc(peek()))) break;
                }

                consume();
            }
        }

        Token lexString() noexcept;
    };
} // namespace lexer
