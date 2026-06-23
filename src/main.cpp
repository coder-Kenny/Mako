#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <chrono>

#include "lexer.hpp"

std::vector<char> loadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};

    auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), static_cast<std::streamsize>(size));

    // We append 16 '\0' bytes as padding to allow safe over-read in the lexer.
    // Lexer may read past 'end' but never more than 16 bytes.
    buffer.insert(buffer.end(), 16, '\0');

    return buffer;
}

int main() {
    auto source = loadFile("test.mako");
    if (source.empty()) return 1;

    lexer::Lexer lex(source.data(), source.data() + (source.size() - 16));

    std::vector<lexer::Token> tokens;
    tokens.reserve(4096);

    auto start_time = std::chrono::high_resolution_clock::now();

    lexer::Token tok;
    do {
        tok = lex.nextToken();
        tokens.push_back(tok);
    } while (tok.tok_kind != lexer::TokenKind::EndOfFile);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    std::cout << "\n--- All tokens issued individually: ---\n";
    for (const auto& t : tokens) {
        std::cout << lexer::toString(t) << "\n";
    }

    std::cout << "\nDuration of pure lexing: " << duration.count() << " ms\n";

    return 0;
}

