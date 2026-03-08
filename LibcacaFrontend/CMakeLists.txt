set(LIB_NAME cacacamera)

add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(CACA REQUIRED caca)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libswscale
    libavutil)

# libslang2-dev
# pkg_check_modules(SLANG REQUIRED IMPORTED_TARGET slang)

find_package(X11 REQUIRED COMPONENTS Xinerama)

set(SOURCES
    videostream.c
    renderer.c
    audiostream.c
    tools.c
    main.c)

target_sources(${LIB_NAME} PRIVATE ${SOURCES})

# main gui library
target_link_libraries(${LIB_NAME}
    PRIVATE
    ${CACA_LIBRARIES}
    # PkgConfig::SLANG
    ${X11_LIBRARIES}
    ${X11_Xinerama_LIB}
    video_converter
    avutil # need for 'error adding symbols: DSO missing from command line'
    Threads::Threads
)

# Directory include
target_include_directories(${LIB_NAME} PRIVATE
    ${CACA_INCLUDE_DIRS}
)

# link video streams
if (USE_VIDEO_BACKEND_V4L2)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        video_stream
    )
elseif (USE_VIDEO_BACKEND_UVC)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        ${LIBUSB_LIBRARIES}
        ${LIBUVC_LIBRARIES}
        video_stream
    )
endif ()

# link audio streams
if (USE_AUDIO_BACKEND_PULSE OR USE_AUDIO_BACKEND_PIPEWIRE OR USE_AUDIO_BACKEND_ALSA)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        audio_stream
    )
    include_directories(${PIPEWIRE_INCLUDE_DIRS})
endif ()
