from conan import ConanFile
from conan.tools.files import chdir, copy, mkdir
from os.path import join
from conan.tools.gnu import PkgConfigDeps

class BasicConanfile(ConanFile):
    name = "catui"
    version = "0.1.2"
    description = "IPC based application library"
    license = "MIT"
    homepage = "https://gulachek.com"

    def source(self):
        self.run("git clone git@github.com:gulachek/catui.git")

    def requirements(self):
        self.requires('msgstream/0.3.0')
        self.requires('unixsocket/0.1.0')
        self.requires('cjson/1.7.16')

    def build_requirements(self):
        # TODO - node and npm install?
        pass

    def generate(self):
        pc = PkgConfigDeps(self)
        d = 'catui/pkgconfig'
        mkdir(self, d)
        with chdir(self, d):
            pc.generate()

    # This method is used to build the source code of the recipe using the desired commands.
    def build(self):
        with chdir(self, 'catui'):
            self.run("npm install")
            self.run("node make.mjs catui")

    def package(self):
        d = join(self.source_folder, 'catui')
        build = join(d, "build")
        include = join(d, "include")
        copy(self, "*.h", include, join(self.package_folder, "include"))
        copy(self, "libcatui.dylib", build, join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.libs = ["catui"]
