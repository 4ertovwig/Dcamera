cmake_minimum_required(VERSION 3.10)

# this library needed by default for audio streams
set(LIB_NAME_CONVERTER video_converter)

add_library(${LIB_NAME_CONVERTER} SHARED)
set_target_properties(${LIB_NAME_CONVERTER} PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libswscale
    libavutil)

pkg_check_modules(LIBAVFILTER REQUIRED IMPORTED_TARGET
    libavfilter)


set(CONVERTER_SOURCES 
    converter.c
    decoders.c
)

target_link_libraries(${LIB_NAME_CONVERTER}
    PRIVATE
    avcodec
    swscale
    avutil
)

target_sources(${LIB_NAME_CONVERTER} PRIVATE ${CONVERTER_SOURCES})

# frame counter library
if (USE_FRAME_COUNTER)

if (USE_LIBJPEG_TURBO_DECODER)
    add_compile_definitions(USE_LIBJPEG_TURBO)
endif ()

    set(LIB_NAME_FRAME_COUNTER frame_counter)

    add_library(${LIB_NAME_FRAME_COUNTER} STATIC)
    set_target_properties(${LIB_NAME_FRAME_COUNTER} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${LIB_NAME_FRAME_COUNTER}
        PRIVATE
        avfilter
    )

    target_sources(${LIB_NAME_FRAME_COUNTER} PRIVATE frame_counter.c)

endif ()

# libjpeg instead of libav decoder
if (USE_LIBJPEG_TURBO_DECODER)

    pkg_check_modules(JPEG REQUIRED libjpeg)
    set(LIB_NAME_JPEG_TURBO_DECODER jpeg_decoder)

    add_library(${LIB_NAME_JPEG_TURBO_DECODER} SHARED)
    set_target_properties(${LIB_NAME_JPEG_TURBO_DECODER} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${LIB_NAME_JPEG_TURBO_DECODER}
        PRIVATE
        ${JPEG_LIBRARIES}
    )

    include_directories(${JPEG_INCLUDE_DIRS})

    target_sources(${LIB_NAME_JPEG_TURBO_DECODER} PRIVATE jpeg_decoder.c)

endif ()

# noise supression by afftdn
if (USE_FFT_NOISE_SUPRESSION)

    set(LIB_NAME_NOISE_SUPRESSION noise_supression_fft)

    add_library(${LIB_NAME_NOISE_SUPRESSION} SHARED)
    set_target_properties(${LIB_NAME_NOISE_SUPRESSION} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${LIB_NAME_NOISE_SUPRESSION}
        PRIVATE
        avcodec
        swscale
        avutil
    )

    target_sources(${LIB_NAME_NOISE_SUPRESSION} PRIVATE fft_noise_supression.c)
endif ()

# noise supression via speexdsp
if (USE_SPEEX_NOISE_SUPRESSION)

    pkg_check_modules(SPEEX REQUIRED speex)
    pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

    set(LIB_NAME_NOISE_SUPRESSION noise_supression_speex)

    add_library(${LIB_NAME_NOISE_SUPRESSION} SHARED)
    set_target_properties(${LIB_NAME_NOISE_SUPRESSION} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${LIB_NAME_NOISE_SUPRESSION}
        PRIVATE
        ${SPEEXDSP_LIBRARIES}
    )

    target_sources(${LIB_NAME_NOISE_SUPRESSION} PRIVATE speex_noise_supression.c)
endif ()

# library for resampler
if (USE_LIBSOXR_RESAMPLER)

    pkg_check_modules(SOXR REQUIRED soxr)
    set(LIB_NAME_RESAMPLER_SOXR resampler_soxr)

    add_library(${LIB_NAME_RESAMPLER_SOXR} SHARED)
    set_target_properties(${LIB_NAME_RESAMPLER_SOXR} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${LIB_NAME_RESAMPLER_SOXR}
        PRIVATE
        soxr
    )

    target_sources(${LIB_NAME_RESAMPLER_SOXR} PRIVATE soxr_resampler.c)

endif ()
