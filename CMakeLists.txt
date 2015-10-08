cmake_minimum_required(VERSION 3.0)
project(libllsm)

if(NOT MSVC)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fopenmp")
else()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /GL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
endif()

set(LLSM_SRC_SOURCES llsm.c envelope.c)
add_library(llsm STATIC ${LLSM_SRC_SOURCES})

set_target_properties(llsm PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(external/libpyin)
target_link_libraries(llsm PUBLIC pyin)

# Arrange header files for external use
add_custom_command(TARGET llsm POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "include")
add_custom_command(TARGET llsm POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/llsm.h" "include/llsm.h")
file(GLOB_RECURSE LLSM_SRC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "src/*.h")
target_include_directories(llsm INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include")
