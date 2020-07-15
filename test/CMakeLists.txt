enable_testing()
include(GoogleTest)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/RequestTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RouterTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/URITest.cpp)

add_executable(grpcbackend-test ${TEST_SOURCES})
set_property(TARGET grpcbackend-test PROPERTY CXX_STANDARD 14)
target_compile_options(grpcbackend-test PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>)
target_link_libraries(grpcbackend-test PRIVATE grpcbackend_cpp-static gtest gtest_main pthread ZLIB::ZLIB ${CMAKE_DL_LIBS})
