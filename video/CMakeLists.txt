cmake_minimum_required(VERSION 3.10)

set(LIB_NAME video_stream)

add_library(${LIB_NAME} SHARED)
set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (USE_VIDEO_BACKEND_UVC)
    set (VIDEO_BACKEND_SRC ${CMAKE_SOURCE_DIR}/video/uvc_stream.c)
elseif (USE_VIDEO_BACKEND_V4L2)
    set (VIDEO_BACKEND_SRC ${CMAKE_SOURCE_DIR}/video/v4l_stream.c)
endif ()

if (USE_VIDEO_BACKEND_UVC)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        ${LIBUSB_LIBRARIES}
        ${LIBUVC_LIBRARIES}
    )
endif ()

target_link_libraries(${LIB_NAME}
    PRIVATE
    video_converter
)

if (USE_FRAME_COUNTER)
    add_compile_definitions(FRAME_COUNTER)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        frame_counter
    )
endif ()

if (USE_LIBJPEG_TURBO_DECODER)
    add_compile_definitions(USE_LIBJPEG_TURBO)
    target_link_libraries(${LIB_NAME}
        PRIVATE
        jpeg_decoder
    )
endif ()

target_sources(${LIB_NAME} PRIVATE ${VIDEO_BACKEND_SRC})
