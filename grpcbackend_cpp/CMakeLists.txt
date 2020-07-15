set(Protobuf_DEBUG False)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH False)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

GRPC_PB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
    handler.proto
)

SET(GRPCBACKEND_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/http/route/filesystem_route_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http/router.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http/mw/rewrite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http/mw/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http/mw/mime_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/hub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uri.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)
SET(GRPCBACKEND_PUBLIC_HDRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:include>
)
SET(GRPCBACKEND_PRIVATE_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}../ttl/ttl/include/
)

add_library(grpcbackend_cpp-static STATIC ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp-shared SHARED ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp::grpcbackend_cpp ALIAS grpcbackend_cpp-static)
set_target_properties(grpcbackend_cpp-shared PROPERTIES OUTPUT_NAME grpcbackend_cpp)
set_target_properties(grpcbackend_cpp-static PROPERTIES OUTPUT_NAME grpcbackend_cpp)

target_link_libraries(grpcbackend_cpp-static
  date
  libprotobuf.a
  libgrpc++.a
  libgrpc.a
  libgpr.a
  libupb.a
  libcrypto.a
  libssl.a
  libcrypto.a
  libz.a
  libabsl.a
  libcares.a
  libaddress_sorting.a
  libstdc++fs.a
  dl
  pthread
)
target_link_libraries(grpcbackend_cpp-shared
  date
  protobuf
  grpc++
  grpc
  gpr
  upb
  crypto
  ssl
  z
  absl
  cares
  address_sorting
  stdc++fs
  dl
  pthread
)

target_include_directories(grpcbackend_cpp-static PUBLIC ${GRPCBACKEND_PUBLIC_HDRS})
target_include_directories(grpcbackend_cpp-static PRIVATE ${GRPCBACKEND_PRIVATE_HDRS})
target_include_directories(grpcbackend_cpp-shared PUBLIC ${GRPCBACKEND_PUBLIC_HDRS})
target_include_directories(grpcbackend_cpp-shared PRIVATE ${GRPCBACKEND_PRIVATE_HDRS})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    COMPONENT dev
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.h"
)

install(TARGETS grpcbackend_cpp-static
    DESTINATION "lib"
    COMPONENT dev
)

install(TARGETS grpcbackend_cpp-shared
    DESTINATION "lib"
    COMPONENT main
)

execute_process(
    COMMAND dpkg --print-architecture
    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Debs
set(CPACK_COMPONENTS_ALL dev main)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_NAME "libgrpcbackend_cpp")
set(CPACK_PACKAGE_VENDOR "Dominik Thalhammer")
set(CPACK_PACKAGE_CONTACT "dominik@thalhammer.it")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Grpc Backend")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Thalhammer/grpcbackend_cpp")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dominik Thalhammer <dominik@thalhammer.it>")
set(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${VERSION_COMMIT}")
set(CPACK_DEBIAN_MAIN_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
set(CPACK_DEBIAN_DEV_FILE_NAME "${CPACK_PACKAGE_NAME}-dev_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
set(CPACK_DEBIAN_MAIN_PACKAGE_NAME "libgrpcbackend_cpp")
set(CPACK_DEBIAN_DEV_PACKAGE_NAME "libgrpcbackend_cpp-dev")

if (UNIX)
    #set(CPACK_GENERATOR "DEB;STGZ;TGZ;TZ")
    set(CPACK_GENERATOR "DEB")
endif (UNIX)

include(CPack)