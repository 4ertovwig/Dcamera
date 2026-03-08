
cmake_minimum_required(VERSION 3.10)
if (USE_CRASH_DUMP)

    set(LIB_NAME_CRASH_HANDLER crash_handler)

    add_library(${LIB_NAME_CRASH_HANDLER} SHARED)
    set_target_properties(${LIB_NAME_CRASH_HANDLER} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_sources(${LIB_NAME_CRASH_HANDLER} PRIVATE crash_handler.c)
endif()