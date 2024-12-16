from conans import ConanFile, AutoToolsBuildEnvironment, tools


class HwlocConan(ConanFile):
    name = "hwloc"
    version = "2.10"
    license = "<license>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Gmt here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    default_options = {}
    scm = {
        "type": "git",
        "url": "https://github.com/open-mpi/hwloc.git",
        "revision": "c80630218a1c11e1ccc3fa89155647c737385f59"
    }

    def build(self):
        autotools = AutoToolsBuildEnvironment(self)
        with tools.environment_append(autotools.vars):
            self.run("./autogen.sh")

        autotools.configure(args=['--enable-plugins'])
        autotools.make()

    def package(self):
        autotools = AutoToolsBuildEnvironment(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.libs = ["hwloc"]
        self.cpp_info.libdirs = ['lib']
