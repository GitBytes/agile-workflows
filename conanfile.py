from conans import ConanFile, CMake, tools


class AgileWorkflowsConan(ConanFile):
    name = "agile-workflows"
    version = "0.0.1"
    license = "APACHE 2.0"
    author = "<Put your name here> <And your email here>"
    # url = "<Package recipe repository url here, for issues about the package>"
    description = "The AGILE Workflows"
    # topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    # options = {"shared": [True, False], "fPIC": [True, False]}
    # default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        self.options["gtest"].build_gmock = False
        self.options["gtest"].no_main = True
        self.options["rapidcheck"].enable_gtest = True

    def requirements(self):
        self.requires('gmt/2.0.0@user/stable')
        self.requires('shad/1.0.0@user/stable')
        self.requires('gtest/1.11.0')
        self.requires('rapidcheck/cci.20210702')
        self.requires('libtorch/1.11.0@user/stable')
        self.requires('pytorch_scatter/2.0.9@user/stable')
        self.requires('pytorch_sparse/0.7.0@user/stable')

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        pass

    def package_info(self):
        pass
