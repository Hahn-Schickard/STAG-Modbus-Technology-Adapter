cmake_minimum_required(VERSION 3.8)
project(PackageTest CXX)

set(VERBOSE_FILE_INCLUSION ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(example example.cpp)

target_link_libraries(example PUBLIC
                              ${CONAN_LIBS}
)

set_target_properties(example PROPERTIES
                              CXX_STANDARD 17
                              CXX_STANDARD_REQUIRED ON
)
