#cmake_minimum_required(VERSION 3.10)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(LIB_NAME qcamera)

add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(SOURCES
    painter.cpp
    videostream.cpp
    audiostream.cpp
    camerawidget.cpp
    main.cpp)

target_sources(${LIB_NAME} PRIVATE ${SOURCES})

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(Threads)

target_include_directories(${LIB_NAME} PUBLIC ${Qt5Widgets_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)

# main gui library
target_link_libraries(${LIB_NAME}
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Concurrent
    Threads::Threads
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
    # TODO
    include_directories(${PIPEWIRE_INCLUDE_DIRS})
endif ()
