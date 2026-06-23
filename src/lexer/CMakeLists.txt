add_library(lexer
        lexer.cpp
        token.cpp
)

target_include_directories(lexer
        PUBLIC
         ${CMAKE_CURRENT_SOURCE_DIR}
)

set_project_warnings(lexer)