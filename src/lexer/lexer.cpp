#include <cstddef>
#include <string_view>

#include "diagnostic.hpp"
#include "lexer.hpp"
#include "token.hpp"

namespace lexer {
    // -----------------------------------------------------------------------------
    //  Skipping Utilities (whitespace & comments)
    // -----------------------------------------------------------------------------
    void Lexer::skipNewLine() noexcept {
        switch (peek()) {
            case '\n':
            case '\f':
                ++state_.buffer.cur;
                ++state_.pos.line;
                state_.pos.col = 1;
                break;

            case '\r':
                ++state_.buffer.cur;
                ++state_.pos.line;
                state_.pos.col = 1;

                // windows line ending: \r\n
                if (peek() == '\n')
                    ++state_.buffer.cur;
                break;

            default:
                __builtin_unreachable();
        }
    }

    void Lexer::skipWhiteSpace() noexcept {
        static constexpr int TAB_SIZE = 4;
        switch (peek()) {
            case ' ':
            case '\v':
                advance();
                break;

            case '\t':
                // rounds the column up to the next multiple of 4 (tab stop)
                ++state_.buffer.cur;
                state_.pos.col = state_.pos.col + (TAB_SIZE - (state_.pos.col % TAB_SIZE));
                break;

            case '\f':
            case '\n':
            case '\r':
                skipNewLine();
                break;

            default:
                __builtin_unreachable();
        }
    }

    void Lexer::skipLineComment() noexcept {
        advance(2); // skip the line comment marker '//'

        while (peek() != 0 && !isNewLine(to_uc(peek())))
            advance();
    }

    bool Lexer::skipBlockComment(const char* start, std::size_t col) noexcept {
        advance(2); // skip the block comment marker '/*'

        while (peek() != 0) {
            unsigned char uc = to_uc(peek());

            if (uc == '*' && peek(1) == '/') {
                advance(2); // skip the closing block comment marker '*/'
                return true;
            }

            if (isNewLine(uc)) skipNewLine();
            else advance();
        }

        errors.push_back(makeErrorToken(ErrorKind::UnterminatedBlockComment, start, col));
        return false;
    }

    // -----------------------------------------------------------------
    // Keyword and Identifier Lexing
    // -----------------------------------------------------------------
    Token Lexer::lexIdentifier() noexcept {
        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        while (isIdPart(to_uc(peek()))) consume();

        std::string_view lexeme{start, static_cast<std::size_t>(state_.buffer.cur - start)};
        TokenKind tok_kind = getKeywordKind(lexeme);

        return makeToken(tok_kind, start, col);
    }

    // -----------------------------------------------------------------
    // Operator Lexing
    // -----------------------------------------------------------------
    Token Lexer::lexOperator() noexcept {
        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        char c = consume();

        switch (c) {
            case '+':
                return makeToken(TokenKind::Plus, start, col);

            case '-':
                if (match('>')) return makeToken(TokenKind::Arrow, start, col);
                return makeToken(TokenKind::Minus, start, col);

            case '*':
                return makeToken(TokenKind::Star, start, col);

            case '/':
                return makeToken(TokenKind::Slash, start, col);

            case '%':
                return makeToken(TokenKind::Percent, start, col);

            case '=':
                if (match('=')) return makeToken(TokenKind::EqualEqual, start, col);
                if (match('>')) return makeToken(TokenKind::FatArrow, start, col);
                return makeToken(TokenKind::Equal, start, col);

            case '!':
                if (match('=')) return makeToken(TokenKind::BangEqual, start, col);
                return makeToken(TokenKind::Bang, start, col);

            case '<':
                if (match('<')) return makeToken(TokenKind::LessLess, start, col);
                if (match('=')) return makeToken(TokenKind::LessEqual, start, col);
                return makeToken(TokenKind::Less, start, col);

            case '>':
                if (match('>')) return makeToken(TokenKind::GreaterGreater, start, col);
                if (match('=')) return makeToken(TokenKind::GreaterEqual, start, col);
                return makeToken(TokenKind::Greater, start, col);

            case '&':
                if (match('&')) return makeToken(TokenKind::AmpAmp, start, col);
                return makeToken(TokenKind::Amp, start, col);

            case '|':
                if (match('|')) return makeToken(TokenKind::PipePipe, start, col);
                return makeToken(TokenKind::Pipe, start, col);

            case '^':
                return makeToken(TokenKind::Caret, start, col);

            case '~':
                return makeToken(TokenKind::Tilde, start, col);

            case '.':
                return makeToken(TokenKind::Dot, start, col);

            default:
                __builtin_unreachable();
        }
    }

    // -----------------------------------------------------------------
    // Number Lexing
    // -----------------------------------------------------------------
    void Lexer::consumeExponent(bool& had_exponent, Diagnostic& diag) noexcept {
        advance(); // skip the exponent marker

        if (had_exponent) {
            diag.recordFirstError(ErrorKind::MultipleExponents, state_.pos.col);
            advance(); // skip the exponent marker (again)
            return;
        }

        had_exponent = true;

        void(match('+') || match('-')); // consume the operator (if available)

        if (!isDigit(to_uc(peek())))
            diag.recordFirstError(ErrorKind::TrailingExponent, state_.pos.col);
        else {
            while (true) {
                unsigned char uc = to_uc(peek());
                if (isDigit(uc)) {
                    consume();
                } else if (uc == '_') {
                    handleUnderscore(diag);
                    consume();
                } else {
                    break;
                } // end of the exponent
            }
        }

        if (!isNumberTerminator(to_uc(peek())))
            diag.recordFirstError(ErrorKind::InvalidExponent, state_.pos.col);
    }

    Token Lexer::lexDecimal(const char* start, std::size_t col) noexcept {
        Diagnostic diag;

        bool had_decimal = false;
        bool had_exponent = false;

        if (peek() == '0' && isDigit(to_uc(peek(1))))
            diag.recordFirstError(ErrorKind::LeadingZeroInInteger, state_.pos.col);

        while (true) {
            unsigned char uc = to_uc(peek());

            if (isNumberTerminator(uc)) break;

            if (isDigit(uc)) {
                consume();
                continue;
            }

            if (uc == '.') {
                if (had_decimal) {
                    diag.recordFirstError(ErrorKind::MultipleDecimalPoints, state_.pos.col);
                }
                else {
                    had_decimal = true;
                    if (!isDigit(to_uc(peek(1))))
                        diag.recordFirstError(ErrorKind::TrailingDecimalPoint, state_.pos.col);
                }
                consume();
                continue;
            }

            if (uc == 'e' || uc == 'E') {
                consumeExponent(had_exponent, diag);
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

        if (diag.had_err) return reportError(diag.err_kind, start, diag.err_col);
        return makeToken(had_decimal || had_exponent ? TokenKind::FloatLiteral : TokenKind::IntegerLiteral, start, col);
    }

    Token Lexer::lexNumber() noexcept {
        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        if (peek() == '0') {
            char next = peek(1);
            if (next == 'b' || next == 'B') return lexBinary(start, col);
            if (next == 'x' || next == 'X') return lexHex(start, col);
            if (next == 'o' || next == 'O') return lexOctal(start, col);
        }

        return lexDecimal(start, col);
    }

    // -----------------------------------------------------------------
    // Char Lexing
    // -----------------------------------------------------------------
    Token Lexer::lexChar() noexcept {
        Diagnostic diag;

        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        advance(); // skip opening '

        if (peek() == '\'') {
            consume();
            return reportError(ErrorKind::EmptyCharLiteral, start, col);
        }

        if (peek() == '\\') matchAndConsumeEscape(diag);
        else consume(); // normal char

        if (!diag.had_err && peek() == '\'') {
            consume();
            return makeToken(TokenKind::CharLiteral, start, col);
        }

        while (true) {
            unsigned char uc = to_uc(peek());
            if (isNewLine(uc) || uc == '\'' || uc == 0) break;
            consume();
        }

        if (peek() != '\'')
            diag.recordFirstError(ErrorKind::UnterminatedCharLiteral, state_.pos.col);
        else {
            diag.recordFirstError(ErrorKind::InvalidCharLiteral, state_.pos.col);
            consume(); // skip ending '
        }

        return reportError(diag.err_kind, start, col);
    }

    // -----------------------------------------------------------------
    // String Lexing
    // -----------------------------------------------------------------
    Token Lexer::lexString() noexcept {
        Diagnostic diag;

        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        advance(); // skip opening "

        while (true) {
            unsigned char uc = to_uc(peek());

            if (isNewLine(uc) || uc == '\"' || uc == 0) break;

            if (uc == '\\') {
                matchAndConsumeEscape(diag);
                continue;
            }

            consume();
        }

        if (peek() == '"') consume(); // skip ending "
        else
            diag.recordFirstError(ErrorKind::UnterminatedStringLiteral, col);

        if (diag.had_err) return reportError(diag.err_kind, start, diag.err_col);
        return makeToken(TokenKind::StringLiteral, start, col);
    }

    // -----------------------------------------------------------------
    // Error Handling
    // -----------------------------------------------------------------
    Token Lexer::lexIllegalChars() noexcept {
        // single-line tokens: store start pointer and start column.
        const char* start = state_.buffer.cur;
        std::size_t col = state_.pos.col;

        while (isIllegal(to_uc(peek()))) consume();

        return reportError(ErrorKind::IllegalCharacters, start, col);
    }


    Lexer::Lexer(const char* begin, const char* end) : state_{{begin, end}, {1, 1}} {}

    Token Lexer::nextToken() noexcept {
        while (true) {
            unsigned char uc = to_uc(peek());

            // end of input
            if (uc == 0) return makeToken(TokenKind::EndOfFile, state_.buffer.cur, state_.pos.col);

            // whitespace
            if (isWhiteSpace(uc)) {
                skipWhiteSpace();
                continue;
            }

            // identifier/Keyword
            if (isIdBegin(uc)) return lexIdentifier();

            // number
            if (isDigit(uc)) return lexNumber();
            if (uc == '.' && isDigit(to_uc(peek(1)))) return lexNumber();

            // char
            if (uc == '\'') return lexChar();

            // string
            if (uc == '"') return lexString();

            // line comment //
            if (uc == '/' && peek(1) == '/') {
                skipLineComment();
                continue;
            }

            // block comment /* ... */
            if (uc == '/' && peek(1) == '*') {
                // single-line tokens: store start pointer and start column.
                const char* start = state_.buffer.cur;
                std::size_t col = state_.pos.col;

                if (skipBlockComment(start, col)) continue;
                return makeToken(TokenKind::Error, start, col);
            }

            // punctuator
            if (isPunctuator[uc] != TokenKind::None) {
                TokenKind kind = isPunctuator[uc];

                // single-line tokens: store start pointer and start column.
                const char* start = state_.buffer.cur;
                std::size_t col = state_.pos.col;

                consume(); // consume the punctuator

                return makeToken(kind, start, col);
            }

            // operator
            if (isOperatorBegin(uc)) return lexOperator();

            // illegal character
            return lexIllegalChars();
        }
    }
} // namespace lexer