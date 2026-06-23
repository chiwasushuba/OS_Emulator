add_library(cpu
    cpu_core.cpp
    cpu_manager.cpp
)

target_link_libraries(cpu
    PUBLIC
    process
)