cmake_minimum_required(VERSION 3.10)

project(Dcamera
    VERSION 1.0.0
    DESCRIPTION "Capture a video stream from a camera and an audio stream from a microphone"
    LANGUAGES C CXX
)

set(CMAKE_AUTOMOC ON)
find_package(PkgConfig REQUIRED)

option(USE_VIDEO_BACKEND_V4L2 "Use v4l2 backend for video from usb camera. This type of stream used /dev/videoX" OFF)
option(USE_VIDEO_BACKEND_UVC "Use libuvc backend for video from usb camera. This type of stream used usb vid/pid" ON)
option(USE_AUDIO_BACKEND_ALSA "Use alsa backend for audio from usb camera" ON)
option(USE_AUDIO_BACKEND_PULSE "Use pulseaudio backend for audio from usb camera" OFF)
option(USE_AUDIO_BACKEND_PIPEWIRE "Use pipewire backend for audio from usb camera" OFF)
option(USE_FRONTEND_QT "Use Qt framework as fronted. Default ON" ON)
option(USE_FRONTEND_X11 "Use Xlib as frontend. Default OFF" OFF)
option(USE_FRONTEND_WAYLAND "Use wayland client as fronted. Default OFF" OFF)
option(USE_FRONTEND_CACA "Use libcaca library as fronted. Default OFF" OFF)
option(USE_LIBSOXR_RESAMPLER "Use libsoxr as resampler for audio streams. TODO: correctly dont work for different input streams params" ON)
option(USE_FRAME_COUNTER "Use frame counter from camera" ON)
option(USE_LIBJPEG_TURBO_DECODER "Use libjpeg turbo decoder instead of libav decoder" ON)
option(USE_FFT_NOISE_SUPRESSION "Use ffmpeg noice supression" OFF)
option(USE_SPEEX_NOISE_SUPRESSION "Use noice supression from speexdsp library" ON)
option(USE_CLANG_COMPILER "Use clang and clang++ as compiler" ON)
option(USE_PROFILING "Build with profiling support" ON)
option(USE_CRASH_DUMP "Use gdb as stacktrace informator" ON)

if (USE_CLANG_COMPILER)
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
    add_compile_definitions(USE_CLANG)
endif ()

if (USE_PROFILING)
    # Compiler flags for perf analyzer
    add_compile_options(-g -fno-omit-frame-pointer)
endif ()

# Sanitizers:
#######################################################
# # For undefined sanitizer
# # WARNING: address sanitizer crush app, but i can't fix it
# add_compile_options(-fsanitize=undefined -g -O1)
# add_link_options(-fsanitize=undefined)

# # For thread sanitizer
# add_compile_options(-fsanitize=thread -g -O1)
# add_link_options("-fsanitize=thread")

# # Need to igrore some spurious cases
# add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins -g -O2
#     -fsanitize-ignorelist=${CMAKE_SOURCE_DIR}/msan_ignore.txt)
# add_link_options("-fsanitize=memory")
#######################################################

if (USE_VIDEO_BACKEND_V4L2 AND USE_VIDEO_BACKEND_UVC)
    message(FATAL_ERROR "Only one video backend must has been used")
endif ()

if (USE_AUDIO_BACKEND_PIPEWIRE AND USE_AUDIO_BACKEND_PULSE)
    message(FATAL_ERROR "Only one audio backend must has been used")
elseif (USE_AUDIO_BACKEND_PIPEWIRE AND USE_AUDIO_BACKEND_ALSA)
    message(FATAL_ERROR "Only one audio backend must has been used")
elseif (USE_AUDIO_BACKEND_PIPEWIRE AND USE_AUDIO_BACKEND_ALSA)
    message(FATAL_ERROR "Only one audio backend must has been used")
endif ()

if (USE_FFT_NOISE_SUPRESSION AND USE_SPEEX_NOISE_SUPRESSION)
    message(FATAL_ERROR "Only one supression method has been used")
endif ()

if (USE_VIDEO_BACKEND_UVC OR USE_VIDEO_BACKEND_V4L2)
    # add spectial directory for raw video streams
    add_subdirectory(video)
endif ()

if (USE_VIDEO_BACKEND_UVC)
    add_compile_definitions(VIDEO_BACKEND_UVC)
    pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)
    message(STATUS "Found libusb: ${LIBUSB_LIBRARIES}")
    pkg_check_modules(LIBUVC libuvc)
    message(STATUS "Found libuvc: ${LIBUVC_LIBRARIES}")
elseif (USE_VIDEO_BACKEND_V4L2)
    add_compile_definitions(VIDEO_BACKEND_V4L2)
endif ()

# add special directory for audio/video transformations
add_subdirectory(media)

if (USE_AUDIO_BACKEND_PULSE OR USE_AUDIO_BACKEND_PIPEWIRE OR USE_AUDIO_BACKEND_ALSA)
    add_subdirectory(audio)
endif ()

if (USE_AUDIO_BACKEND_PULSE)
    add_compile_definitions(AUDIO_BACKEND_PULSE)
elseif (USE_AUDIO_BACKEND_PIPEWIRE)
    add_compile_definitions(AUDIO_BACKEND_PIPEWIRE)
elseif (USE_AUDIO_BACKEND_ALSA)
    add_compile_definitions(AUDIO_BACKEND_ALSA)
endif ()

# add special directory for crash handler and dump
if (USE_CRASH_DUMP)
    add_subdirectory(crash_dump)
    add_compile_definitions(CRASH_DUMP)
endif ()

# add spectial directory for args processing and logging
add_subdirectory(common)

# Frontends
if (USE_FRONTEND_QT)
    add_subdirectory(QtFrontend)
    add_compile_definitions(FRONTEND_QT)
    # use just for Qt, other methods did not help
    add_compile_options(-fPIC)
endif ()

if (USE_FRONTEND_X11)
    add_subdirectory(X11Frontend)
    add_compile_definitions(FRONTEND_X11)
endif ()

if (USE_FRONTEND_WAYLAND)
    add_subdirectory(WaylandFrontend)
    add_compile_definitions(FRONTEND_WAYLAND)
endif ()

if (USE_FRONTEND_CACA)
    add_subdirectory(LibcacaFrontend)
    add_compile_definitions(FRONTEND_CACA)
endif ()

add_executable(${PROJECT_NAME}
    app.cpp
    main.cpp)

if (USE_CRASH_DUMP)
    target_link_libraries(${PROJECT_NAME}
    PRIVATE
    crash_handler)
endif ()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_NAME}_common)

if (USE_FRONTEND_QT)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        qcamera)
endif ()

if (USE_FRONTEND_WAYLAND)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        wcamera)
endif ()

if (USE_FRONTEND_CACA)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        cacacamera)
endif ()

if (USE_FRONTEND_X11)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        xcamera)
endif ()

# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)