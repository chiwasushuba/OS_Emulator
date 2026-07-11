add_library(cpu
    cpu_core.cpp
    cpu_manager.cpp
)

target_include_directories(cpu
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cpu
    PUBLIC
    process
)