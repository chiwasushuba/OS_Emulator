add_library(core
    process_generator.cpp
    report_generator.cpp
)

target_link_libraries(core
    PUBLIC
    process
    cpu
    scheduler
)