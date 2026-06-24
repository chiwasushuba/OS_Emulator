add_library(scheduler
    scheduler.cpp
    fcfs.cpp
    round_robin.cpp
)

target_include_directories(scheduler
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(scheduler
    PUBLIC process
    PUBLIC cpu
)