cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(Modbus_Technology_Adapter CONFIG REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example Modbus_Technology_Adapter::Modbus_Technology_Adapter GTest::GTest)

add_custom_command(TARGET example POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                        ${PROJECT_SOURCE_DIR}/../example_config.json
                        $<TARGET_FILE_DIR:example>
                    COMMENT "Copying configuration file."
                        VERBATIM
)
