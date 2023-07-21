cmake_minimum_required(VERSION 3.6)

#@+ ======================== User MODULE NAME configuration ============================
set(THIS Adapter)
#@- =========================== END OF USER CONFIGURATION ===============================

set(MODULE ${PROJECT_NAME}_${THIS})

list(APPEND THIS_INCLUDES_DIRS
            ${INCLUDES_DIR}/${THIS}
)

file(GLOB sources_list "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

INCLUDE_DIRS_AND_FILES("${THIS_INCLUDES_DIRS}" THIS_INCLUDES)

add_library(${MODULE})

target_sources(${MODULE}
               PRIVATE
               ${THIS_INCLUDES}
               ${sources_list}
)

#@+ ======================== User DEPENDENCIES configuration ============================
target_link_libraries(${MODULE}
                      PRIVATE #Private dependencies
                        libmodbus::libmodbus
                        nlohmann_json::nlohmann_json
                      PUBLIC  #Public dependencies
                        Technology_Adapter_Interface::Technology_Adapter_Interface
)
#@- =========================== END OF USER CONFIGURATION ===============================

target_include_directories(${MODULE} INTERFACE
                           $<BUILD_INTERFACE:${INCLUDES_DIR}/${THIS}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
)

target_compile_features(${MODULE} PUBLIC cxx_std_17)

PRINT_TARGET_PROPERTIES(${MODULE})

install(
    TARGETS ${MODULE}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES ${THIS_INCLUDES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)
