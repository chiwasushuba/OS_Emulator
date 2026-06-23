add_library(cli
    console.cpp
    commands.cpp
    command_handler.cpp
)

target_link_libraries(cli
    PRIVATE
    core
)
