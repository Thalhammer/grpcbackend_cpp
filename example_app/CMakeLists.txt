add_executable(example_app
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set_property(TARGET example_app PROPERTY CXX_STANDARD 14)
target_compile_options(example_app PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>)
target_link_libraries(example_app PRIVATE grpcbackend_cpp-static pthread ZLIB::ZLIB ${CMAKE_DL_LIBS})
