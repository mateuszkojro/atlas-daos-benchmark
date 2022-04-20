#!/usr/bin/python3
import sys
import subprocess
import os
import argparse
import shutil


class logging:
    @staticmethod
    def info(msg):
        print(msg)


class Utils:
    @staticmethod
    def running_in_docker():
        return os.path.exists("/.dockerenv")

    @staticmethod
    def program_avaliable(cmd):
        return shutil.which(cmd) is not None

    @staticmethod
    def load_flags(file):
        with open(file, "r") as f:
            content = f.read()
            lines = content.splitlines()
        return lines


class Program:
    def __init__(self, program_executable, check_success=True, working_dir=".", shell=False) -> None:
        if not Utils.program_avaliable(program_executable):
            raise RuntimeError(f"Program not found: {program_executable}")
        self._program_executable = program_executable
        self._working_dir = working_dir
        self.check_success = check_success

    @property
    def program_executable(self) -> str:
        return self._program_executable

    def run_command(self, parameters, **kwargs):
        command = [self.program_executable]
        command += parameters
        logging.info(f"=== Running: {command} ===")
        result = subprocess.run(command, cwd=self._working_dir, **kwargs)
        if self.check_success:
            result.check_returncode()
        return result


class CMake(Program):
    def __init__(self, builddir="build", cmake_program='cmake'):
        super().__init__(cmake_program)
        self.builddir = builddir

    def clean(self):
        shutil.rmtree(self.builddir)

    def configure(self, path=".", defines=None):
        command = []
        if defines is not None:
            command += defines
        command += ["-B", self.builddir, path]
        return self.run_command(command)

    def build(self, core_count=str(os.cpu_count() - 1)):
        return self.run_command(["--build", self.builddir, "-j", core_count])

    def target(self, target):
        return self.run_command(["--target", target, self.builddir])

    def __call__(self, subcommands):
        return self.run_command(subcommands)


class Docker(Program):
    def __init__(self, docker_program='docker'):
        super().__init__(docker_program)

    def exec(self, container, program, interactive=False):
        command = ["exec"]
        if interactive:
            command += ["-it"]
        command += [container, program]
        return self.run_command(command)

    def run(self, image, name="temporary_container", parameters=""):
        return self.run_command(["run", "-it", image, "--name", name, parameters])

    def create(self, container, name="dummy"):
        return self.run_command(["create", "--name", name, container])

    def copy(self, src, dst):
        return self.run_command(["cp", src, dst])

    def build(self, path, tag="tempoary"):
        return self.run_command(["build", "-t", tag, path])

    def remove(self, image):
        return self.run_command(["rm", image])

    def push(self):
        pass

    def __call__(self, subcommands):
        return self.run_command(subcommands)


class SSH(Program):
    def __init__(self, host, ssh_program='ssh'):
        super().__init__(ssh_program)
        self.host = host

    def exec(self, command):
        self.run_command([self.host, command])


class SCP(Program):
    def __init__(self, scp_program='scp'):
        super().__init__(scp_program)

    def copy(self, src, dst):
        self.run_command([src, dst])


def run_on_cluster(cmake: CMake):
    cmake.configure()
    cmake.build()
    SCP().copy("./build/bench/bench", "olsky-02:~/")
    SSH("olsky-02").exec("./bench")


def docker_compile_and_copy(docker: Docker):
    image_name = "adb"
    container_name = "dummy"
    out_folder = "/tmp/"
    docker.create(image_name, container_name)
    docker.copy(f"{container_name}:/app/build/bench/bench", out_folder)
    docker.remove(container_name)
    docker.copy(f"{out_folder}/bench", "daos-client:/")
    docker.exec("daos-client", "./bench")

def compile_daos(daos_dir):
    scons = Program("scons-3", working_dir=daos_dir)
    cpu_count = os.cpu_count() - 2 if os.cpu_count() is not None else 1
    scons.run_command(["install", "-j", str(cpu_count),
                        "--build-deps=yes", "--config=force"])

if __name__ == '__main__':
    cmake_defines = Utils.load_flags("cmake.in")
    if Utils.running_in_docker():
        cmake_defines += [f'-DDAOS_DIR={"/daos/install"}']
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--configure", action='store_true')
    parser.add_argument("--build", action='store_true')
    parser.add_argument("--docker_build", action='store_true')
    parser.add_argument("--clean", action='store_true')
    parser.add_argument("--cluster_run", action='store_true')
    parser.add_argument("--build_in_docker", action='store_true')
    parser.add_argument("--build_daos", action='store_true')
    args = parser.parse_args()

    cmake = CMake()

    if args.build_daos:
        compile_daos("./lib/daos-cxx/lib/daos")

    if args.clean:
        cmake.clean()

    if args.configure:
        cmake.configure(defines=cmake_defines)

    if args.build:
        cmake.build()

    if Utils.running_in_docker():
        sys.exit(0)

    docker = Docker()
    if args.docker_build:
        docker.build(".", "adb")

    if args.cluster_run:
        run_on_cluster(cmake)

    if args.build_in_docker:
        docker_compile_and_copy(docker)
