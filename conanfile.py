from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import load, copy, collect_libs
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
import re
import os


def to_camel_case(input: str):
    words = input.replace("_", " ").split()
    return '_'.join(word.capitalize() for word in words)


class PackageConan(ConanFile):
    # @+ START USER META CONFIG
    license = "Apache 2.0"
    description = "Modbus Technology Adapter implementation"
    topics = ("stag","cpp17","modbus","technology-adapter")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False],
               "fPIC": [True, False]}
    default_options = {"shared": True,
                       "fPIC": True}
    default_user = "Hahn-Schickard"
    # @- END USER META CONFIG
    exports_sources = [
        "cmake*",
        "includes*",
        "sources*",
        "CMakeLists.txt",
        "conanfile.py",
        # @+ START USER EXPORTS
        # @- END USER EXPORTS
    ]
    generators = "CMakeDeps"
    short_paths = True

    @property
    def cwd(self):
        return os.path.dirname(os.path.realpath(__file__))

    def set_name(self):
        content = load(self, path=os.path.join(self.cwd, 'CMakeLists.txt'))
        name = re.search('set\(THIS (.*)\)', content).group(1)
        self.name = name.strip().lower()

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, "17")

    def requirements(self):
        # @+ START USER REQUIREMENTS
        self.requires("libmodbus/3.1.8")
        self.requires("nlohmann_json/3.11.1")
        self.requires('hasll/[~0.3]@hahn-schickard/stable', headers=True,
                      libs=True, transitive_headers=True, transitive_libs=True)
        self.requires(
            "index/[~0.0]@hahn-schickard/stable", headers=True, transitive_headers=True)
        self.requires(
            "threadsafe_containers/[~0.8]@hahn-schickard/stable", headers=True, transitive_headers=True)
        self.requires(
            "technology_adapter_interface/[~0.2]@hahn-schickard/stable", headers=True, transitive_headers=True)
        self.test_requires("gtest/[~1.11]")
        # @- END USER REQUIREMENTS

    def configure(self):
        # @+ START USER REQUIREMENTS OPTION CONFIGURATION
        pass
        # @- END USER REQUIREMENTS OPTION CONFIGURATION

    def layout(self):
        cmake_layout(self)

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables['STATIC_CODE_ANALYSIS'] = False
        tc.variables['RUN_TESTS'] = False
        tc.variables['COVERAGE_TRACKING'] = False
        tc.variables['CMAKE_CONAN'] = False
        tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0077"] = "NEW"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, pattern='LICENSE', dst='licenses', src=self.cwd)
        copy(self, pattern='NOTICE', dst='licenses', src=self.cwd)
        copy(self, pattern='AUTHORS', dst='licenses', src=self.cwd)

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
        self.cpp_info.set_property("cmake_find_mode", "both")
        # @+ START USER DEFINES
        project_name = to_camel_case(self.name)
        # @- END USER DEFINES
        self.cpp_info.set_property("cmake_file_name", project_name)
        cmake_target_name = project_name + "::" + project_name
        self.cpp_info.set_property("cmake_target_name", cmake_target_name)
