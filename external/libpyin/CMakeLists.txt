cmake_minimum_required(VERSION 3.0)
project(libpyin)

if(NOT MSVC)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
else()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /GL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
endif()

aux_source_directory(. PYIN_SRC_SOURCES)
add_library(pyin STATIC ${PYIN_SRC_SOURCES})

set_target_properties(pyin PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(external/libgvps)
target_link_libraries(pyin PUBLIC gvps)

# Arrange header files for external use
add_custom_command(TARGET pyin POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "include")
add_custom_command(TARGET pyin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/pyin.h" "include/pyin.h")
file(GLOB_RECURSE PYIN_SRC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "src/*.h")
target_include_directories(pyin INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include")
