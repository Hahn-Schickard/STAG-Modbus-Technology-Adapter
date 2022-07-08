[![pipeline status](https://git.hahn-schickard.de/hahn-schickard/software-sollutions/application-engineering/internal/cpp-utilities/badges/master/pipeline.svg)](https://git.hahn-schickard.de/hahn-schickard/software-sollutions/application-engineering/internal/cpp-utilities/-/commits/master)
[![coverage report](https://git.hahn-schickard.de/hahn-schickard/software-sollutions/application-engineering/internal/cpp-utilities/badges/master/coverage.svg)](https://git.hahn-schickard.de/hahn-schickard/software-sollutions/application-engineering/internal/cpp-utilities/-/commits/master)

<img src="docs/code_documentation/vendor-logo.png" alt="" width="200"/>

# PROJECT_NAME

## Description

## Documentation

If you want to have the latest documentation with your changes locally, you can generate it with [Doxygen](https://github.com/doxygen/doxygen) from sources by running the following:

```bash
doxygen utility/Doxyfile
```

This will generate an html like documentation at `[PROJECT_ROOT]/docs/code_documentation/html`. To use it open the `[PROJECT_ROOT]/docs/code_documentation/html/index.html` file with your browser.

## Dependencies
### Required

* cmake - build system generator
* python3 - used by utilities
* conan - dependency handler, see [SSO Wiki](https://ssowiki.hsg.privat/en/Softwareentwicklung/Cpp/Conan_Package_Manager) for installation

### Optional

* clang-format - to use formatting tools
* clang-tidy - to use static code analysis
* doxygen - to generate documentation from code

### Conan packages
 * [gtest/1.11.0](https://conan.io/center/gtest?version=1.11.0)

## Visual Studio Code Support

### Recommended Plugins:
* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)  - provides linking to *inellisense*  and code debuggers
* [C++ Intellisense](https://marketplace.visualstudio.com/items?itemName=austin.code-gnu-global) - provides code highlighting and quick navigation
* [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) - provides CMake highlighting, configuring, building
* [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) - provides code formatting
* [Test Explorer UI](https://marketplace.visualstudio.com/items?itemName=hbenl.vscode-test-explorer) - provides test runner integration
* [C++ TestMate](https://marketplace.visualstudio.com/items?itemName=matepek.vscode-catch2-test-adapter) - provides google-test framework adapter for Test Explorer UI

## Building the project

To build the project [CMake](https://cmake.org/) project makefile generation as well as integrated testing and linting tools.

We recommend to create a directory for project makefiles and binaries:

```bash
mkdir build && cd build
```

Once in this new **build** directory, generate the project makefiles:

```bash
cmake ..
```

Once makefiles have been generated, build the project either in **Debug** configuration:

```bash
cmake --build . --target all --config Debug --
```

or **Release** configuration:

```bash
cmake --build . --target all --config Release --
```

Once the project is built, it is also possible to use the integrated tests runner to run the provided tests:

```bash
ctest --verbose
```

## Project utility tools

This project comes with integrated utility scripts written in python3 to check code coverage with **gcov** and **lcov**, check for memory leaks with **valgrind** and generate documentation with **Doxygen**
