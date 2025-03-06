from conans import ConanFile, CMake
from conans.tools import load
import os
import re

def get_version():
    # This method opens the .release file that is required to be within the
    # project top-level directory in the repository.
    # The file should contain as a first line, the text:
    #
    # release={major}.{minor}.{patch}
    #
    # where major, minor and patch are integers

    try:
        content = load( ".release" )
        return re.search( "release=([0-9.]*)", content ).group(1)
    except Exception as e:
        return None

class SkaMidCbfFodmGen(ConanFile):
    name = "ska-mid-cbf-fodm-gen"
    version = get_version()
    license = "BSD-3-Clause"
    author = "John So (john.so@mda.space)"

    settings = { "os": ["Linux"],
                 "compiler" : [ "gcc" ],
                 "build_type": [ "Debug", "Release" ],
                 "arch" : [ "x86", "x86_64", "armv8" ]
                }
    
    options = {"shared": [True, False], "fPIC": [True, False]}

    default_options = {"shared": False, "fPIC": True}
    
    generators = "cmake"
    
    no_copy_source = True

    def requirements(self):
        # to support multi-precision floating point
        self.requires("boost/1.71.0")
        if ( self.settings.build_type == "Debug" ):
            self.requires("gtest/1.15.0")

    def build(self):
        cmake = CMake(self)
        if ( self.in_local_cache ):
            cmake.configure(defs={"TARGET_ARCH": f"{self.settings.arch}"}, source_folder=self.source_folder )
        else:
            cmake.configure(defs={"TARGET_ARCH": f"{self.settings.arch}"})
        cmake.build()

    def package(self):
        self.copy("*.h", src="src", dst="include", keep_path=False)
        self.copy("*.a", src="lib", dst="lib", keep_path=False)
        
    def package_info(self):
        self.cpp_info.name = self.name
        self.cpp_info.libs = [self.name]