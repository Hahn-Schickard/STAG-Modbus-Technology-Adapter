from conans import ConanFile, CMake, tools
from conans.tools import load
import re
import os
import shutil


class PackageConan(ConanFile):
    license = "Apache 2.0"
    topics = ("", "")
    build_requires = "gtest/[~1.11]"
    requires = [
        "Threadsafe_Containers/[~0.6]@hahn-schickard/stable",
        "Technology_Adapter_Interface/[~0.1]@hahn-schickard/stable",
        "libmodbus/3.1.8",
    ]
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False],
               "fPIC": [True, False]}
    default_options = {"shared": True,
                       "fPIC": True}
    default_user = "Hahn-Schickard"
    exports_sources = [
        "cmake*",
        "includes*",
        "sources*",
        "unit_tests*",
        "CMakeLists.txt",
        "conanfile.py",
        "LICENSE",
        "NOTICE",
        "AUTHORS"
    ]
    _cmake = None
    generators = ['cmake', 'cmake_paths', 'cmake_find_package']

    @property
    def cwd(self):
        return os.path.dirname(os.path.realpath(__file__))

    def set_name(self):
        content = load(os.path.join(self.cwd, 'CMakeLists.txt'))
        name = re.search('set\(THIS (.*)\)', content).group(1)
        self.name = name.strip()

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        self._cmake.verbose = True
        self._cmake.definitions['STATIC_CODE_ANALYSIS'] = False
        self._cmake.definitions['RUN_TESTS'] = False
        self._cmake.definitions['USE_CONAN'] = True
        self._cmake.configure()
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy(pattern='LICENSE', dst='licenses', src=self.cwd)
        self.copy(pattern='NOTICE', dst='licenses', src=self.cwd)
        self.copy(pattern='AUTHORS', dst='licenses', src=self.cwd)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        self.output.info('Collected libs: \n{}'.format(
            '\n'.join(self.cpp_info.libs)))
