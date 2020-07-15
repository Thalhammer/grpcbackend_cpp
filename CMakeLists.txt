cmake_minimum_required(VERSION 3.10)
project(grpcbackend)

find_package (Git)
if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty=-d
        OUTPUT_VARIABLE VERSION)
else (GIT_FOUND)
      set(VERSION "v0.0.0-0-00000000-d")
endif (GIT_FOUND)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-([0-9]+).*" "\\1" VERSION_COMMIT "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version.cpp.in
#                ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)
#set(version_file "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

# Testing
enable_testing()
include(GoogleTest)

find_package(ZLIB REQUIRED)

add_subdirectory(date)
add_subdirectory(grpcbackend_cpp)
add_subdirectory(test)
add_subdirectory(example_app)