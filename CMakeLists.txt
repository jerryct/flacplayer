cmake_minimum_required(VERSION 3.10)

project(flacplayer CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS Off)
set(DEFAULT_COPTS "-fno-omit-frame-pointer" "-g")# "-Wall" "-Wextra")
set(DEFAULT_LINKOPTS "")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
  list(APPEND DEFAULT_COPTS
    $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fprofile-instr-generate>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fcoverage-mapping>
    $<$<CXX_COMPILER_ID:GNU>:-ftree-vectorize>
    $<$<CXX_COMPILER_ID:GNU>:-fopt-info-vec-optimized>
    -march=nehalem
  )
  list(APPEND DEFAULT_LINKOPTS
    $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fprofile-instr-generate>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fcoverage-mapping>
  )
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.*|aarch64")
else()
  message(WARNING "Value of CMAKE_SYSTEM_PROCESSOR (${CMAKE_SYSTEM_PROCESSOR}) is unknown")
endif()

add_compile_options(
  ${DEFAULT_COPTS}
)
link_libraries(
  ${DEFAULT_LINKOPTS}
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
add_subdirectory(../benchmark _build/benchmark)
add_subdirectory(../googletest _build/googletest)
add_subdirectory(../fmt _build/fmt)

add_library(plac
  audio_buffer.h
  audio_format.h
  bit_cast.h
  conditions.h
  file_desc.h
  flow_control.h
  stream.cpp
  stream.h
)
target_link_libraries(plac PUBLIC FLAC pthread fmt)

add_executable(flacplayer
  alsa_audio_device.cpp
  alsa_audio_device.h
  linux_player.cpp
)
target_link_libraries(flacplayer PRIVATE asound plac)

add_executable(unit_tests
  audio_buffer_unit_test.cpp
  audio_format_unit_test.cpp
  flac_decode_unit_test.cpp
  stream_unit_test.cpp
)
target_link_libraries(unit_tests PRIVATE plac gtest_main)

add_executable(benchmarks
  audio_buffer_benchmark.cpp
)
target_link_libraries(benchmarks PRIVATE plac benchmark::benchmark)
