cmake_minimum_required(VERSION 3.6)

#@+ ======================== User MODULE NAME configuration ============================
set(THIS Example_Runner)
#@- =========================== END OF USER CONFIGURATION ===============================

set(MODULE ${PROJECT_NAME}_${THIS})

file(GLOB sources_list "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_executable(${MODULE})

target_sources(${MODULE} PRIVATE
                       ${sources_list}
)

#@+ ======================== User DEPENDENCIES configuration ============================
target_link_libraries(${MODULE}
                      PRIVATE #Private dependencies
                        libmodbus::libmodbus
                        ${PROJECT_NAME}_Adapter
                        GTest::GTest
                      PUBLIC  #Public dependencies
)
#@- =========================== END OF USER CONFIGURATION ===============================

target_compile_features(${MODULE} PUBLIC cxx_std_17)
