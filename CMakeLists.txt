cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package (Git)
if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty=-d
        OUTPUT_VARIABLE VERSION
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
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

project(grpcbackend
    VERSION ${VERSION_SHORT}
)

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

execute_process(
    COMMAND dpkg --print-architecture
    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Debs
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_NAME "libgrpcbackend")
set(CPACK_PACKAGE_VENDOR "Dominik Thalhammer")
set(CPACK_PACKAGE_CONTACT "dominik@thalhammer.it")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Grpc Backend")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Thalhammer/grpcbackend_cpp")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dominik Thalhammer <dominik@thalhammer.it>")
set(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${VERSION_COMMIT}")

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLE "Build example_app" ON)

add_subdirectory(grpcbackend_cpp)
if(BUILD_TESTS)
    add_subdirectory(test)
endif(BUILD_TESTS)
if(BUILD_EXAMPLE)
    add_subdirectory(example_app)
endif(BUILD_EXAMPLE)

if (UNIX)
    #set(CPACK_GENERATOR "DEB;STGZ;TGZ;TZ")
    set(CPACK_GENERATOR "DEB")
endif (UNIX)

include(CPack)