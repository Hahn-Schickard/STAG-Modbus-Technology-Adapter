cmake_minimum_required(VERSION 3.24.0)

option(CMAKE_CONAN "Enables integeated conan package manager module. (Requires conan and python3 to be installed)" ON)

if(CMAKE_CONAN)
        set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)
endif()

#@+ ======================== User PROJECT_NAME configuration ============================
set(THIS Modbus_Technology_Adapter)
#@- =========================== END OF USER CONFIGURATION ===============================

project(${THIS} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDES_DIR "${PROJECT_SOURCE_DIR}/includes")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(DOCS_DIR "${PROJECT_SOURCE_DIR}/docs/code_documentation")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VERBOSE_FILE_INCLUSION "Prints all included header files" ON)
option(STATIC_CODE_ANALYSIS "Enable Code Linter" ON)
option(RUN_TESTS "Enables Unit tests runner (Requires GTest framework)" ON)
option(COVERAGE_TRACKING "Enable code test coverage tracking with gcov" ON)

include(GNUInstallDirs)
include(macros)
include(linter)
include(compiler_settings)

#@+ ====================== User CMAKE_OPTIONS configuration ========================
# User defined options and cmake variables are set here
#@- ===================== END OF USER CONFIGURATION ================================

#@+ ==================== User CMAKE_INCLUDES configuration =======================
# User defined cmake module includes go here
#@- ======================== END OF USER CONFIGURATION ============================

#@+ ======================== User PACKAGES configuration ===========================
list(APPEND DEPENDENCIES_LIST
        Index
        Technology_Adapter_Interface
        Threadsafe_Containers
        nlohmann_json
        libmodbus
)
#@- ========================= END OF USER CONFIGURATION ============================

FIND_INSTALLED_DEPENDENCIES("${DEPENDENCIES_LIST}")

add_subdirectory(sources)
if(RUN_TESTS)
        enable_testing()
        add_subdirectory(unit_tests)
endif()

#@+ ====================== User CMAKE configuration =========================
# General user defined CMake configurtaions are set here
#@- ===================== END OF USER CONFIGURATION =========================

WRITE_PKG_CONFIG_IN_FILE(${PROJECT_NAME})

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${THIS_PACKAGE_NAME}/
)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_PREFIX}
)
