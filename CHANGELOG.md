# Changelog
## [0.1.14] - 2022.11.08
### Changed
  - document, package and lint jobs to not use any dependencies
  - remove needs for lint and document jobs
  - run-valgrind.py utility to accept any number of target args instead of only 1

### Added
 - entry points for fedora based jobs

## [0.1.13] - 2022.11.02
### Fixed 
 - typos in contribution rules

### Changed
 - standard development image to debian buster
 - run-lcov.py to accept multiple arguments for the target binary

### Added
 - RUN_TESTS = False cmake definition, to disable test building while packaging
 - source and binary file upload to artifactory
 - conan uploads for all os targets

### Removed
 - slow artifact compression

## [0.1.12] - 2022.10.17
### Fixed
 - package jobs to use project root instead of conan dir
 - pages jobs for multi-target releases to create correct publishes

## [0.1.11] - 2022.10.17
### Added
 - CMake variants integration section to Readme

### Changed
 - `conan_install.cmake` file to use official [`conan.cmake`](https://github.com/conan-io/cmake-conan) module
 - `pip3 install conan --upgrade` into `pip3 install -U conan==1.53.0` to avoid upgrading into conan 2.0
 - `conanfile.py` to be placed into project root directory instead of conan/
 - Gitlab Issue Templates to use Hahn-Schickard Labels

### Removed 
 - `*/conan/*` and `conanfile.txt` from Doxygen pattern excludes
 - conan directory

## [0.1.10] - 2022.09.28
### Fixed 
 - spelling mistakes in `run-clang-format.py`
 
### Changed
 - lint job to only list formatting suggestions and linted files
 - removed `List` specifier for `.create-static-website.py`
 - removed `List` specifier for `.getVersionFromTag.py`
 - removed `List` specifier for `.run-clang-format.py`
 - removed `List` specifier for `.run-lcov.py`
 - removed `List` specifier for `.run-valgrind.py`
 - refactored `print_diff()` function in `.run-clang-format.py` to always print the differences

## [0.1.9] - 2022.09.19
### Added
 - gitlab issue templates
 - standard cmake-variants.yaml
 - status message when code linting is disabled

### Changed
 - conan_install.cmake to use our custom implementation
 - conanfile.py to no run units tests and code linting
 - integration test to use RunEnvironment helper
 - ci-cd configuration to use debian-bullseye as a base image
 
### Removed
 - clang--tidy use-no-discard check

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
