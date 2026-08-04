add_library(memory
    paging_allocator.cpp
)

target_include_directories(memory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(memory
    PUBLIC
    process
)
