from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.files import copy

class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    build_policy = "missing"

    def requirements(self):
        self.requires("nlohmann_json/3.11.2")
        self.requires("simdjson/3.6.1")
        self.requires("boost/1.83.0")
        self.requires("onetbb/2021.10.0")
        self.requires("expat/2.5.0")
        self.requires("zlib/[>=1.2.0]")
        # self.requires("json-schema-validator/2.2.0")
        # self.requires("parallel-hashmap/1.37")
        # self.requires("fmt/10.1.1")

    def build_requirements(self):
        self.tool_requires("cmake/3.27.1")
        self.requires("gtest/1.14.0")
        # self.build_requires("gcc/12.2.0")
        
    def generate(self):
        print("conanfile.py: IDE include dirs:")
        for lib, dep in self.dependencies.items():
            if not lib.headers:
                continue
            for inc in dep.cpp_info.includedirs:
                print("\t" + inc)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
