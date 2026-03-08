set(LIB_NAME xcamera)

add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(XCB_IMAGE REQUIRED xcb-image)
pkg_check_modules(XCB_ATOM REQUIRED xcb-atom)

find_package(Threads REQUIRED)

set(SOURCES
    videostream.c
    audiostream.c
    renderer.c
    main.c)

target_sources(${LIB_NAME} PRIVATE ${SOURCES})

# main gui library
target_link_libraries(${LIB_NAME}
    PRIVATE
    ${XCB_LIBRARIES}
    ${XCB_IMAGE_LIBRARIES}
    ${XCB_ATOM_LIBRARIES}
    Threads::Threads
    video_converter
    avutil
)

target_include_directories(${LIB_NAME} PRIVATE
    ${XCB_INCLUDE_DIRS}
    ${XCB_IMAGE_INCLUDE_DIRS}
    ${XCB_ATOM_INCLUDE_DIRS}
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
