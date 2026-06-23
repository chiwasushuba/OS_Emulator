add_library(scheduler
    scheduler.cpp
    fcfs.cpp
    round_robin.cpp
)

target_link_libraries(scheduler
    PUBLIC process
    PUBLIC cpu
)