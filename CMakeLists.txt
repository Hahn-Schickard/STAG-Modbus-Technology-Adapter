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
option(RUN_TESTS "Enables Unit tests runner (Requires GTest framework)" ON)
option(COVERAGE_TRACKING "Enable code test coverage tracking with gcov" ON)

if(NOT WIN32)
    option(STATIC_CODE_ANALYSIS "Enable Code Linter" ON)
endif()

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

find_package(GTest REQUIRED)

#@+ ======================== User PACKAGES configuration ===========================
find_package(Const_String)
find_package(HSCUL REQUIRED)
find_package(Index)
find_package(Technology_Adapter_Interface)
find_package(Threadsafe_Containers)
find_package(nlohmann_json)
find_package(libmodbus)
#@- ========================= END OF USER CONFIGURATION ============================

if(CMAKE_CONAN)
    include(${CMAKE_BINARY_DIR}/conan/build/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake)
endif()

add_subdirectory(sources)
if(RUN_TESTS)
    enable_testing()
    add_subdirectory(unit_tests)
endif()
