## [0.1.8] - 2022.09.15
### Added 
 - RunEnvironment helper to conan/test_package/conanfile.py 
 - CMAKE_VARIABLES to disable static code analysis for Fedora 34 builds

### Changed
 - conan_instal.cmake to use official conan.cmake module
 - clang-tidy ConstantCase formatting
 - clang-tidy ConstantPointerParameterCase formatting
 - clang-tidy ConstexprVariableCase formatting
 - clang-tidy LocalConstantCase formatting
 - clang-tidy LocalConstantPointerCase formatting

### Removed
 - List specifications from python utility scripts

## [0.1.7] - 2022.09.08
### Added
 - needs specifier to check_code_coverage
 - needs specifier to lint
 - needs specifier to pages

### Changed
 - needs specifier for document job
 - clang-tidy linting rules

### Removed
 - Custom_ContentFormat_Types.md
 - doc_main_page.md
 - doxygen_special_command_cheatsheet.md

## [0.1.6] - 2022.08.28
### Changed
 - gitlab ci/cd pipeline configuration to use coverage specifier
 - gitlab ci/cd pipeline job names
 - Readme Documentation section to point to correct Doxyfile location
 - Readme Recommended Plugins section to Doxygen and Spell-Checker plugins
 - Readme Project utility tools section to clarify utility script usage

## [0.1.5] - 2022.07.08
### Added
 - GPLv3 License
 - BSL-1.0 License
 - missing End Of File new lines
 - new contribution rules to contributing guide
 - release initiation process to contribution guide

### Changed
 - python into python3
 - contributing guide to clarify general contribution procedure

### Removed
 - .git from .template-ignores
 - trailing whitespaces in tempalte files

### Fixed
 - missing subprocess import for python based utils
 - wrong call to run-valgrind.py in analyze_memory_usage job

## [0.1.4] - 2022.06.10
### Added
 - missing terminating line for conan/test_package/example.cpp
 - missing terminating line for conan/test_package/CMakeLists.txt

### Removed
 - unused whitespace in conan/test_package/CMakeLists.txt

## [0.1.3] - 2022.06.09
### Changed
 - conan integration test
 - run-*.py scripts to be self sufficient
 - CI scripts to be no longer visible to normal user

### Removed
 - overly specific information in conan package
 - utility.py module

## [0.1.2] - 2022.06.08
### Removed
 - trailing whitespaces from root CMakeLists.txt

## [0.1.1] - 2022.06.07
### Added
 - .git to .template-ignores

## [0.1.0] - 2022.06.07
### Added
 - .template-ignores file
 - Description tag for README.md

### Changed
 - docs/index.html to docs/.index.html
 - docs/w3.css to docs/.w3.css

## [0.0.1] - 2022.06.02
### Added
 - Authors
 - Readme
 - Contributing rules

## [Initial Release] - 2021.04.14
