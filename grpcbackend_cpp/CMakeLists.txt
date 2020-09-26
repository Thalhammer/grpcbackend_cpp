set(Protobuf_DEBUG False)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH False)

include(GitUtils)
git_clone(
    PROJECT_NAME    date
    GIT_URL         https://github.com/HowardHinnant/date.git
    GIT_BRANCH      master
    DIRECTORY       ${CMAKE_BINARY_DIR}
    QUIET
)
git_clone(
    PROJECT_NAME    ttl
    GIT_URL         https://github.com/Thalhammer/ttl.git
    GIT_BRANCH      master
    DIRECTORY       ${CMAKE_BINARY_DIR}
)

find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
include(GenerateGRPC)

GRPC_PB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
    handler.proto
)

SET(GRPCBACKEND_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/route/filesystem_route_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/router.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/rewrite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/mw/mime_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/json_rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket/connection.cpp
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
    ${CMAKE_BINARY_DIR}/ttl/ttl/include/
    ${CMAKE_BINARY_DIR}/date/include/
)

add_library(grpcbackend_cpp-static STATIC ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp-shared SHARED ${GRPCBACKEND_SRCS})
add_library(grpcbackend_cpp::grpcbackend_cpp ALIAS grpcbackend_cpp-static)
set_target_properties(grpcbackend_cpp-shared PROPERTIES OUTPUT_NAME grpcbackend_cpp)
set_target_properties(grpcbackend_cpp-static PROPERTIES OUTPUT_NAME grpcbackend_cpp)
set_target_properties(grpcbackend_cpp-shared PROPERTIES VERSION ${VERSION_SHORT} SOVERSION ${VERSION_MAJOR})
target_compile_features(grpcbackend_cpp-shared PUBLIC cxx_std_17)
target_compile_features(grpcbackend_cpp-static PUBLIC cxx_std_17)

target_link_libraries(grpcbackend_cpp-static PUBLIC gRPC::grpc++ stdc++fs)
target_link_libraries(grpcbackend_cpp-shared PUBLIC gRPC::grpc++ stdc++fs)

target_include_directories(grpcbackend_cpp-static PUBLIC ${GRPCBACKEND_PUBLIC_HDRS})
target_include_directories(grpcbackend_cpp-static PRIVATE ${GRPCBACKEND_PRIVATE_HDRS})
target_include_directories(grpcbackend_cpp-shared PUBLIC ${GRPCBACKEND_PUBLIC_HDRS})
target_include_directories(grpcbackend_cpp-shared PRIVATE ${GRPCBACKEND_PRIVATE_HDRS})
target_compile_definitions(grpcbackend_cpp-static PUBLIC "PICOJSON_USE_INT64=1")
target_compile_definitions(grpcbackend_cpp-shared PUBLIC "PICOJSON_USE_INT64=1")
target_compile_definitions(grpcbackend_cpp-static PUBLIC "HAS_UNCAUGHT_EXCEPTIONS=1")
target_compile_definitions(grpcbackend_cpp-shared PUBLIC "HAS_UNCAUGHT_EXCEPTIONS=1")
target_compile_options(grpcbackend_cpp-static PRIVATE -Wno-unused-parameter)
target_compile_options(grpcbackend_cpp-shared PRIVATE -Wno-unused-parameter)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    COMPONENT grpcbackend_dev
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.h"
)

install(TARGETS grpcbackend_cpp-static
    EXPORT grpcbackend_cpp-export
    DESTINATION "lib"
    COMPONENT grpcbackend_dev
)

install(TARGETS grpcbackend_cpp-shared
    EXPORT grpcbackend_cpp-export
    DESTINATION "lib"
    COMPONENT grpcbackend
)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/grpcbackend_cpp)

install(EXPORT grpcbackend_cpp-export
    FILE grpcbackend_cppTargets.cmake
    NAMESPACE grpcbackend_cpp::
    DESTINATION ${INSTALL_CONFIGDIR}
    COMPONENT grpcbackend_dev
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/grpcbackend_cppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/grpcbackend_cppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/grpcbackend_cppConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/grpcbackend_cppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/grpcbackend_cppConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
    COMPONENT grpcbackend_dev
)


set(CPACK_DEBIAN_GRPCBACKEND_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_FILE_NAME "${CPACK_PACKAGE_NAME}-dev_${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_PACKAGE_NAME "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev" PARENT_SCOPE)
set(CPACK_DEBIAN_GRPCBACKEND_DEV_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} grpcbackend_dev grpcbackend PARENT_SCOPE)