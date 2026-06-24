add_library(cli
    console.cpp
    commands.cpp
    command_handler.cpp
)

target_include_directories(cli
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cli
    PRIVATE
    core
)
