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

add_custom_command(TARGET example POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${PROJECT_SOURCE_DIR}/../example_config.json
                        $<TARGET_FILE_DIR:example>
                    COMMENT "Copying configuration file."
                        VERBATIM
)
