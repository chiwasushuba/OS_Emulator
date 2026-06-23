add_library(kernel
    kernel.cpp
)

target_link_libraries(kernel
    PRIVATE
    cli
    core
    cpu
    process
    scheduler
    config
)