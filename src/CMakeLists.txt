add_subdirectory(cli)
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(cpu)
add_subdirectory(kernel)
add_subdirectory(memory)
add_subdirectory(process)
add_subdirectory(scheduler)

add_executable(os_emulator
    main.cpp
)

target_link_libraries(os_emulator
    PRIVATE kernel
)