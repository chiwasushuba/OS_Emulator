add_library(cli
    console.cpp
    commands.cpp
    command_handler.cpp
)

target_include_directories(cli
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Include kernel headers (kernel links cli, so we can't link kernel here — just include)
target_include_directories(cli
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel
)

target_link_libraries(cli
    PRIVATE
    core
)
