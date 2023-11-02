from conan import ConanFile
from conan.tools.files import chdir, copy
from os.path import join

class BasicConanfile(ConanFile):
    name = "catui"
    version = "0.1.0"
    description = "IPC based application library"
    license = "MIT"
    homepage = "https://gulachek.com"

    def source(self):
        self.run("git clone git@github.com:gulachek/catui.git")

    def requirements(self):
        pass

    def build_requirements(self):
        # TODO - node and npm install?
        pass

    def generate(self):
        pass

    # This method is used to build the source code of the recipe using the desired commands.
    def build(self):
        with chdir(self, 'catui'):
            self.run("npm install")
            self.run("node make.js catui")

    def package(self):
        d = join(self.source_folder, 'catui')
        build = join(d, "build")
        include = join(d, "include")
        copy(self, "*.h", include, join(self.package_folder, "include"))
        copy(self, "libcatui.dylib", build, join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.libs = ["catui"]
