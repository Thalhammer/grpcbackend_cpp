set(Protobuf_DEBUG False)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH False)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

GRPC_PB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
    handler.proto
)

SET(GRPCBACKEND_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/route/filesystem_route_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/router.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/rewrite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/mime_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/json_rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket/hub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uri.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)
SET(GRPCBACKEND_PUBLIC_HDRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)
SET(GRPCBACKEND_PRIVATE_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}../ttl/ttl/include/
    ${PROTOBUF_INCLUDE_DIRS}
)

add_library(grpcbackend_cpp-static STATIC ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp-shared SHARED ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp::grpcbackend_cpp ALIAS grpcbackend_cpp-static)
set_target_properties(grpcbackend_cpp-shared PROPERTIES OUTPUT_NAME grpcbackend_cpp)
set_target_properties(grpcbackend_cpp-static PROPERTIES OUTPUT_NAME grpcbackend_cpp)
set_target_properties(grpcbackend_cpp-shared PROPERTIES VERSION ${VERSION_SHORT} SOVERSION ${VERSION_MAJOR})

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
target_compile_definitions(grpcbackend_cpp-static PUBLIC "PICOJSON_USE_INT64=1")
target_compile_definitions(grpcbackend_cpp-shared PUBLIC "PICOJSON_USE_INT64=1")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    COMPONENT grpcbackend_dev
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.h"
)

install(TARGETS grpcbackend_cpp-static
    DESTINATION "lib"
    COMPONENT grpcbackend_dev
)

install(TARGETS grpcbackend_cpp-shared
    DESTINATION "lib"
    COMPONENT grpcbackend
)

set(CPACK_DEBIAN_GRPCBACKEND_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_FILE_NAME "${CPACK_PACKAGE_NAME}-dev_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_PACKAGE_NAME "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} grpcbackend_dev grpcbackend PARENT_SCOPE)