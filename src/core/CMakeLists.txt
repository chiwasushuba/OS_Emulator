add_library(core
    process_generator.cpp
    report_generator.cpp
    process_viewer.cpp
)

target_include_directories(core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core
    PUBLIC
    process
    cpu
    scheduler
)