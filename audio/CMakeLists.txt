cmake_minimum_required(VERSION 3.10)

set(LIB_BACKEND audio_stream)
set(LIB_DEVICES audio_devices)

add_compile_options("-fPIC")

add_library(${LIB_BACKEND} SHARED)
set_target_properties(${LIB_BACKEND} PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(${LIB_DEVICES} SHARED)
set_target_properties(${LIB_DEVICES} PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)

if (USE_AUDIO_BACKEND_PULSE)
    set (AUDIO_BACKEND_SRC
        ${CMAKE_SOURCE_DIR}/audio/pulse_stream.c)
    set (AUDIO_DEVICES_SRC
        ${CMAKE_SOURCE_DIR}/audio/pulseaudio_devices.c)
    add_compile_definitions(AUDIO_BACKEND_PULSE)
elseif (USE_AUDIO_BACKEND_PIPEWIRE)
    set (AUDIO_BACKEND_SRC
        ${CMAKE_SOURCE_DIR}/audio/pipewire_stream.c)
    set (AUDIO_DEVICES_SRC
        ${CMAKE_SOURCE_DIR}/audio/pipewire_audio_devices.c)
    add_compile_definitions(AUDIO_BACKEND_PIPEWIRE)
elseif (USE_AUDIO_BACKEND_ALSA)
    set (AUDIO_BACKEND_SRC
        ${CMAKE_SOURCE_DIR}/audio/alsa_stream.c)
    set (AUDIO_DEVICES_SRC
        ${CMAKE_SOURCE_DIR}/audio/alsa_devices.c)
    add_compile_definitions(AUDIO_BACKEND_ALSA)
endif ()

#main library with libalsa backend
if (USE_AUDIO_BACKEND_ALSA)
    find_package(ALSA REQUIRED)
    include_directories(alsa_echoloop PRIVATE ${ALSA_INCLUDE_DIR})
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        ${ALSA_LIBRARIES}
    )
    target_link_libraries(${LIB_DEVICES}
        PRIVATE
        ${ALSA_LIBRARIES}
    )
endif ()

# main library with pulseaudio backend
if (USE_AUDIO_BACKEND_PULSE)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse)
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        ${PULSEAUDIO_LIBRARIES}
    )
    target_link_libraries(${LIB_DEVICES}
        PRIVATE
        ${PULSEAUDIO_LIBRARIES}
    )
endif ()

# main library with pipewire backend
if (USE_AUDIO_BACKEND_PIPEWIRE)
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    pkg_check_modules(GLIB2 REQUIRED glib-2.0)
    pkg_check_modules(WIREPLUMBER IMPORTED_TARGET REQUIRED wireplumber-0.4)
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        ${PIPEWIRE_LIBRARIES}
        PkgConfig::WIREPLUMBER
    )
    target_link_libraries(${LIB_DEVICES}
        PRIVATE
        ${PIPEWIRE_LIBRARIES}
        PkgConfig::WIREPLUMBER
    )
    include_directories(${GLIB2_INCLUDE_DIRS})
    include_directories(${WIREPLUMBER_INCLUDE_DIRS})
    include_directories(${PIPEWIRE_INCLUDE_DIRS})
endif ()

# link noise supression
if (USE_FFT_NOISE_SUPRESSION)
    add_compile_definitions(USE_FFT_NOISE_SUPRESSION)
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        noise_supression_fft
    )
endif ()

if (USE_SPEEX_NOISE_SUPRESSION)
    add_compile_definitions(USE_SPEEX_NOISE_SUPRESSION)
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        noise_supression_speex
    )
endif ()

# link resampler
if (USE_LIBSOXR_RESAMPLER)
    add_compile_definitions(USE_LIBSOXR)
    target_link_libraries(${LIB_BACKEND}
        PRIVATE
        resampler_soxr
    )
endif()

target_sources(${LIB_BACKEND} PRIVATE ${AUDIO_BACKEND_SRC})
target_sources(${LIB_DEVICES} PRIVATE ${AUDIO_DEVICES_SRC})
