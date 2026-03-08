#cmake_minimum_required(VERSION 3.10)

add_library(${PROJECT_NAME}_common SHARED
    args.c
    logging.c
)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}_common
    PRIVATE Threads::Threads
    audio_devices
)
