from conans import ConanFile, CMake, tools
from conans.tools import load
import re
import os
import shutil


class PackageConan(ConanFile):
    license = "Apache 2.0"
    topics = ("lwm2m", "server", "coap")
    build_requires = "gtest/1.10.0"
    requires = [
        "asio/1.16.0",
        "HaSLL/0.2.2@hahn-schickard/stable",
        "Threadsafe_Containers/0.2.2@hahn-schickard/stable"
    ]
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False],
               "fPIC": [True, False],
               "LSB_NBO": [True, False]}
    default_options = {"shared": True,
                       "fPIC": True,
                       "LSB_NBO": False}
    default_user = "Hahn-Schickard"
    exports_sources = [
        "../cmake*",
        "../includes*",
        "../sources*",
        "../unit_tests*",
        "../CMakeLists.txt",
        "../conanfile.txt",
        "../loggerConfig.json",
        "../README.md",
        "../LICENSE",
        "../NOTICE",
        "../AUTHORS",
    ]
    _cmake = None

    @property
    def _source_subfolder(self):
        current_file_loc = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(current_file_loc, "..")

    def set_name(self):
        content = load(os.path.join(self._source_subfolder, "CMakeLists.txt"))
        name = re.search("set\(THIS (.*)\)", content).group(1)
        self.name = name.strip()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        self._cmake.verbose = True
        self._cmake.configure(build_dir=os.path.join(
            self.build_folder, "build"))
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        cmake.test()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy(pattern="LICENSE", dst="licenses",
                  src=self._source_subfolder)
        self.copy(pattern="NOTICE", dst="licenses", src=self._source_subfolder)
        self.copy(pattern="AUTHORS", dst="licenses",
                  src=self._source_subfolder)

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = self.name
        self.cpp_info.names["cmake_find_package_multi"] = self.name
        self.cpp_info.libs = tools.collect_libs(self)
