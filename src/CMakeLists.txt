add_subdirectory(lexer)

add_executable(Mako
        main.cpp
)

target_link_libraries(Mako PRIVATE
        lexer
)

set_project_warnings(Mako)