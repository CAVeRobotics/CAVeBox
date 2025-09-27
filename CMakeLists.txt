cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(CAVeBox LANGUAGES CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fdiagnostics-color=always")

add_compile_definitions(
    $<$<CONFIG:Debug>:LOGGER>
)

option(ABSL_PROPAGATE_CXX_STD "Propagate C++ standard to abseil-cpp" ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/CAVeTalk/external/protobuf/third_party/abseil-cpp)

# TODO set hint based on target arch
find_package(SDL2 REQUIRED HINTS "/opt/sysroot-arm64/usr/lib/aarch64-linux-gnu/cmake/SDL2/")
find_package(utf8_range REQUIRED HINTS ${CMAKE_SOURCE_DIR}/external/CAVeTalk/external/protobuf/_build/protobuf-install)
find_package(absl REQUIRED HINTS ${CMAKE_SOURCE_DIR}/external/CAVeTalk/external/protobuf/_build/protobuf-install)
find_package(Protobuf REQUIRED HINTS ${CMAKE_SOURCE_DIR}/external/CAVeTalk/external/protobuf/_build/protobuf-install)

set(SRCS
    ${CMAKE_SOURCE_DIR}/src/cavebox.cc
    ${CMAKE_SOURCE_DIR}/src/game_controller.cc
    ${CMAKE_SOURCE_DIR}/src/logger.cc
    ${CMAKE_SOURCE_DIR}/src/serial.cc
)

set(INC_DIRS
    ${CMAKE_SOURCE_DIR}/inc
)

add_executable(${PROJECT_NAME} ${SRCS})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LOGGER
        $<$<CONFIG:Debug>:LOGGER_LEVEL=LOGGER_LEVEL_DEBUG>
        $<$<CONFIG:Release>:LOGGER_LEVEL=LOGGER_LEVEL_INFO>
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${INC_DIRS}
        ${CMAKE_SOURCE_DIR}/external/asio/asio/include
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/lib/c++/inc
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/lib/common/inc
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/build/CAVeTalk-cpp_protos
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/build/libCAVeTalk-cpp.a
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/build/libCAVeTalk-cpp_messages.a
        ${CMAKE_SOURCE_DIR}/external/CAVeTalk/build/libCAVeTalk-common.a
        protobuf::libprotobuf
)

set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/tools)
set(CPPCHECK_SOURCES ${SRCS})
include(${TOOLS_DIR}/cppcheck/cppcheck.cmake)
set(UNCRUSTIFY_SOURCES ${CPPCHECK_SOURCES})
set(UNCRUSTIFY_INC_DIRS ${INC_DIRS})
include(${TOOLS_DIR}/uncrustify/uncrustify.cmake)