cmake_minimum_required(VERSION 3.15)
project(PresentationBuilder CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- Our own source files, as a library (excludes main.cpp - the app and
# every test executable link against this instead of recompiling/relisting
# every source file individually) ---
file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

# --- Vendored miniz (zip library used for direct .pptx generation) ---
file(GLOB MINIZ_SOURCES ${CMAKE_SOURCE_DIR}/third_party/miniz/*.c)
set_source_files_properties(${MINIZ_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")

add_library(presentation_builder_lib STATIC ${LIB_SOURCES} ${MINIZ_SOURCES})

target_include_directories(presentation_builder_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/miniz
)

if(WIN32)
    target_link_libraries(presentation_builder_lib PUBLIC ws2_32)
endif()

# --- The actual CLI app: just main.cpp + the library ---
add_executable(presentation_builder src/main.cpp)
target_link_libraries(presentation_builder PRIVATE presentation_builder_lib)

# --- Tests (CTest ships with CMake - no new dependency) ---
enable_testing()
add_subdirectory(tests)
