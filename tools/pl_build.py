from enum import Enum
from contextlib import contextmanager
 
class FileType(Enum):
    UNKNOWN = 0
    BATCH = 1
    BASH = 2


class TargetType(Enum):
    NONE = 0
    STATIC_LIBRARY = 0
    DYNAMIC_LIBRARY = 1
    EXECUTABLE = 2


class Compiler(Enum):
    NONE = 0
    MSVC = 1
    CLANG = 2
    GCC = 3


###############################################################################
#                                 Classes                                     #
###############################################################################


class CompilerSettings:
    def __init__(self, name, compiler_type):
        self._name = name
        self._compiler_type = compiler_type
        self._output_directory = "./"
        self._output_binary = ""
        self._output_binary_extension = None
        self._definitions = []
        self._compiler_flags = []
        self._linker_flags = []
        self._include_directories = []
        self._link_directories = []
        self._source_files = []
        self._link_libraries = []
        self._target_type = TargetType.NONE


class CompilerConfiguration:
    def __init__(self, name):
        self._compiler_settings = []
        self._name = name


class Target:
    def __init__(self, name, target_type):
        self._name = name
        self._target_type = target_type
        self._lock_file = 'lock.tmp'
        self._configurations = []


class Project:
    def __init__(self, name):
        self._name = name
        self._win32_script_name = "build_" + name + "_win32"
        self._macos_script_name = "build_" + name + "_macos"
        self._linux_script_name = "build_" + name + "_linux"
        self._main_target_name = ""
        self._working_directory = "./"
        self._targets = []
        self._registered_options = []
        self._registered_flags = []
        self._registered_configurations = []


class BuildContext:
    def __init__(self):
        self._current_project = None
        self._current_target = None
        self._current_configuration = None
        self._current_compiler_settings = None
        self._projects = []
        
_context = BuildContext()

###############################################################################
#                                 Project                                     #
###############################################################################

@contextmanager
def project(name):

    try:
        _context._projects.append(Project(name))
        _context._current_project = _context._projects[-1]
        yield _context._current_project
    finally: 
        _context._current_project = None

def add_configuration(name):
    _context._current_project._registered_configurations.append(name)
    
def set_working_directory(directory):
    _context._current_project._working_directory = directory

def set_main_target(target_name):
    _context._current_project._main_target_name = target_name

def set_build_win32_script_name(script_name):
    _context._current_project._win32_script_name = script_name

def set_build_linux_script_name(script_name):
    _context._current_project._linux_script_name = script_name

def set_build_macos_script_name(script_name):
    _context._current_project._macos_script_name = script_name

###############################################################################
#                                 Target                                      #
###############################################################################

@contextmanager
def target(name, target_type):

    try:
        _context._current_target = Target(name, target_type)
        yield _context._current_target
    finally:
        _context._current_project._targets.append(_context._current_target)
        _context._current_target = None

###############################################################################
#                             Configuration                                   #
###############################################################################

@contextmanager
def configuration(name):
    try:
        config = CompilerConfiguration(name)
        _context._current_configuration = config
        yield _context._current_configuration
    finally:
        _context._current_target._configurations.append(_context._current_configuration)
        _context._current_configuration = None

###############################################################################
#                               Compiler                                      #
###############################################################################

@contextmanager
def compiler(name, compiler_type):

    try:
        compiler = CompilerSettings(name, compiler_type)
        _context._current_compiler_settings = compiler
        yield _context._current_compiler_settings
    finally:
        _context._current_configuration._compiler_settings.append(_context._current_compiler_settings)
        _context._current_compiler_settings = None

def add_source_file(file):
    _context._current_compiler_settings._source_files.append(file)

def add_source_files(*args):
    for arg in args:
        add_source_file(arg)

def add_link_library(library):
    _context._current_compiler_settings._link_libraries.append(library)

def add_link_libraries(*args):
    for arg in args:
        add_link_library(arg)

def add_definition(definition):
    _context._current_compiler_settings._definitions.append(definition)

def add_definitions(*args):
    for arg in args:
        add_definition(arg)

def add_compiler_flag(flag):
    _context._current_compiler_settings._compiler_flags.append(flag)

def add_compiler_flags(*args):
    for arg in args:
        add_compiler_flag(arg)

def add_linker_flag(flag):
    _context._current_compiler_settings._linker_flags.append(flag)

def add_linker_flags(*args):
    for arg in args:
        add_linker_flag(arg)

def add_include_directory(directory):
    _context._current_compiler_settings._include_directories.append(directory)

def add_include_directories(*args):
    for arg in args:
        add_include_directory(arg)

def add_link_directory(directory):
    _context._current_compiler_settings._link_directories.append(directory)

def add_link_directories(*args):
    for arg in args:
        add_link_directory(arg)

def set_output_binary(binary):
    _context._current_compiler_settings._output_binary = binary

def set_output_binary_extension(extension):
    _context._current_compiler_settings._output_binary_extension = extension

def set_output_directory(directory):
    _context._current_compiler_settings._output_directory = directory

###############################################################################
#                               Generation                                    #
###############################################################################

def _comment(message, file_type):
    
    if file_type == FileType.BATCH:
        return "@rem " + message + "\n"

    elif file_type == FileType.BASH:
        return "# " + message + "\n"
    
    return message

def _title(title, file_type):

    line_length = 80
    padding = (line_length  - 2 - len(title)) / 2
    result = _comment("#" * line_length, file_type)
    result += _comment("#" + " " * int(padding) + title + " " * int(padding + 0.5) + "#", file_type)
    result += _comment("#" * line_length, file_type)
    return result

def _new_lines(n):
    return "\n" * n

def _setup_defaults():

    for project in _context._projects:
        for target in project._targets:
            for config in target._configurations:
                for settings in config._compiler_settings:   
                    if settings._output_binary_extension is None:
                        if target._target_type == TargetType.STATIC_LIBRARY:
                            if settings._compiler_type == Compiler.MSVC:
                                settings._output_binary_extension = ".lib"
                            else :
                                settings._output_binary_extension = ".o"
                        elif target._target_type == TargetType.DYNAMIC_LIBRARY:
                            if settings._compiler_type == Compiler.MSVC:
                                settings._output_binary_extension = ".dll"
                            else :
                                settings._output_binary_extension = ".so"
                        elif target._target_type == TargetType.EXECUTABLE:
                            if settings._compiler_type == Compiler.MSVC:
                                settings._output_binary_extension = ".exe"
                            else :
                                settings._output_binary_extension = ""

def generate_macos_build():

    _setup_defaults()

    for project in _context._projects:

        filepath = project._working_directory + "//" + project._macos_script_name + ".sh"
        file_type = FileType.BASH

        with open(filepath, "w") as file:
            buffer = "#!/bin/bash"
            buffer += _new_lines(2)
            buffer += "# colors\n"
            buffer += "BOLD=$'\e[0;1m'\n"
            buffer += "RED=$'\e[0;31m'\n"
            buffer += "RED_BG=$'\e[0;41m'\n"
            buffer += "GREEN=$'\e[0;32m'\n"
            buffer += "GREEN_BG=$'\e[0;42m'\n"
            buffer += "CYAN=$'\e[0;36m'\n"
            buffer += "MAGENTA=$'\e[0;35m'\n"
            buffer += "YELLOW=$'\e[0;33m'\n"
            buffer += "WHITE=$'\e[0;97m'\n"
            buffer += "NC=$'\e[0m'\n"
            buffer += _new_lines(2)
            buffer += _comment('find directory of this script', file_type)
            buffer += "SOURCE=${BASH_SOURCE[0]}\n"
            buffer += 'while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink\n'
            buffer += '  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )\n'
            buffer += '  SOURCE=$(readlink "$SOURCE")\n'
            buffer += '  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located\n'
            buffer += 'done\n'
            buffer += 'DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )\n'
            buffer += _new_lines(2)
            buffer += _comment('make current directory the same as this script', file_type)
            buffer += 'pushd $DIR >/dev/null\n'
            buffer += _comment('get platform & architecture', file_type)
            buffer += 'PLAT="$(uname)"\n'
            buffer += 'ARCH="$(uname -m)"\n'

            buffer += 'PL_CONFIG=' + project._registered_configurations[0] + '\n'
            buffer += 'while getopts ":c:" option; do\n'
            buffer += '   case $option in\n'
            buffer += '   c) # set conf\n'
            buffer += '         PL_CONFIG=$OPTARG;;\n'
            buffer += '     \?) # Invalid option\n'
            buffer += '         echo "Error: Invalid option"\n'
            buffer += '         exit;;\n'
            buffer += '   esac\n'
            buffer += 'done\n'

            buffer += _new_lines(2)
            for register_config in project._registered_configurations:

                # find main target
                target_found = False
                for target in project._targets:
                    if target._target_type == TargetType.EXECUTABLE and target._name == project._main_target_name:
                        for config in target._configurations:
                            if config._name == register_config:
                                for settings in config._compiler_settings:
                                    if settings._compiler_type == Compiler.CLANG:
                                        
                                        buffer += 'if [[ "$PL_CONFIG" == "' + register_config + '" ]]; then\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=0\n'
                                        buffer += 'if lsof | grep -i -q ' + settings._output_binary + '\n'
                                        buffer += 'then\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=1\n'
                                        buffer += 'echo\n'
                                        buffer += 'echo ${BOLD}${WHITE}${RED_BG}--------${GREEN_BG} HOT RELOADING ${RED_BG}--------${NC}\n'
                                        buffer += 'echo\n'
                                        buffer += 'else\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=0\n'
                                        target_found = True
                for target in project._targets:
                    for config in target._configurations:
                        if config._name == register_config:
                            for settings in config._compiler_settings:
                                if settings._compiler_type == Compiler.CLANG:
                                    if target._target_type == TargetType.EXECUTABLE:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                                    elif target._target_type == TargetType.DYNAMIC_LIBRARY:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + '_*' + settings._output_binary_extension + '\n'
                                    elif target._target_type == TargetType.STATIC_LIBRARY:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                if target_found:
                    buffer += "fi\n"
                buffer += _new_lines(2)
                for target in project._targets:

                    for config in target._configurations:

                        if config._name == register_config:

                            for settings in config._compiler_settings:

                                if settings._compiler_type == Compiler.CLANG:

                                    buffer += _title(config._name + " | " + target._name, file_type)
                                    buffer += _new_lines(1)
                                    buffer += _comment('create output directory', file_type)
                                    buffer += 'if ! [[ -d "' + settings._output_directory + '" ]]; then\n'
                                    buffer += '    mkdir "' + settings._output_directory + '"\n'
                                    buffer += 'fi\n'
                                    buffer += 'echo LOCKING > "./' + settings._output_directory + '/' + target._lock_file + '"'
                                    buffer += _new_lines(2)

                                    buffer += 'PL_DEFINES=\n'    
                                    for define in settings._definitions:
                                        buffer += 'PL_DEFINES+=" -D' + define + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_INCLUDE_DIRECTORIES=\n'    
                                    for include in settings._include_directories:
                                        buffer += 'PL_INCLUDE_DIRECTORIES+=" -I' + include + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINK_DIRECTORIES=\n'    
                                    for link in settings._link_directories:
                                        buffer += 'PL_LINK_DIRECTORIES+=" -L' + link + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_COMPILER_FLAGS=\n'    
                                    for flag in settings._compiler_flags:
                                        buffer += 'PL_COMPILER_FLAGS+=" ' + flag + '"\n'
                                    buffer += 'if [[ "$ARCH" == "arm64" ]]; then\n'
                                    buffer += 'PL_COMPILER_FLAGS+=" -arch arm64"\n'
                                    buffer += 'else\n'
                                    buffer += 'PL_COMPILER_FLAGS+=" -arch x86_64"\n'
                                    buffer += 'fi\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINKER_FLAGS=\n'    
                                    for flag in settings._linker_flags:
                                        buffer += 'PL_LINKER_FLAGS+=" -l' + flag + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINK_LIBRARIES=\n'    
                                    for link in settings._link_libraries:
                                        buffer += 'PL_LINK_LIBRARIES+=" -framework ' + link + '"\n'
                                    buffer += _new_lines(1)
                                    buffer += "PL_RESULT=${BOLD}${GREEN}Successful.${NC}"
                                    buffer += _new_lines(1)

                                    if target._target_type == TargetType.STATIC_LIBRARY:
        
                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling...${NC}\n'
                                    
                                        for source in settings._source_files:
                                            buffer += 'clang -c -fPIC $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS ' + source + ' -o "./' + settings._output_directory + '/' + source + '.o"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

                                    elif target._target_type == TargetType.DYNAMIC_LIBRARY:

                                        buffer += 'PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += 'PL_SOURCES+=" ' + source + '"\n'
                                        buffer += _new_lines(1)

                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling and Linking...${NC}\n'
                                        buffer += 'clang -shared -fPIC $PL_SOURCES $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS $PL_INCLUDE_DIRECTORIES $PL_LINK_DIRECTORIES $PL_LINKER_FLAGS $PL_LINK_LIBRARIES -o "./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension +'"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

                                    elif target._target_type == TargetType.EXECUTABLE:

                                        buffer += 'PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += 'PL_SOURCES+=" ' + source + '"\n'

                                        buffer += _new_lines(1)
                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling and Linking...${NC}\n'
                                        buffer += 'clang -fPIC $PL_SOURCES $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS $PL_INCLUDE_DIRECTORIES $PL_LINK_DIRECTORIES $PL_LINKER_FLAGS $PL_LINK_LIBRARIES -o "./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension +'"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

            buffer += 'fi\n'     
            buffer += 'popd >/dev/null'
            file.write(buffer)

def generate_linux_build():

    _setup_defaults()

    for project in _context._projects:

        filepath = project._working_directory + "//" + project._linux_script_name + ".sh"
        file_type = FileType.BASH

        with open(filepath, "w") as file:
            buffer = "#!/bin/bash"
            buffer += _new_lines(2)
            buffer += "# colors\n"
            buffer += "BOLD=$'\e[0;1m'\n"
            buffer += "RED=$'\e[0;31m'\n"
            buffer += "RED_BG=$'\e[0;41m'\n"
            buffer += "GREEN=$'\e[0;32m'\n"
            buffer += "GREEN_BG=$'\e[0;42m'\n"
            buffer += "CYAN=$'\e[0;36m'\n"
            buffer += "MAGENTA=$'\e[0;35m'\n"
            buffer += "YELLOW=$'\e[0;33m'\n"
            buffer += "WHITE=$'\e[0;97m'\n"
            buffer += "NC=$'\e[0m'\n"
            buffer += _new_lines(2)
            buffer += _comment('find directory of this script', file_type)
            buffer += "SOURCE=${BASH_SOURCE[0]}\n"
            buffer += 'while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink\n'
            buffer += '  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )\n'
            buffer += '  SOURCE=$(readlink "$SOURCE")\n'
            buffer += '  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located\n'
            buffer += 'done\n'
            buffer += 'DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )\n'
            buffer += _new_lines(2)
            buffer += _comment('make current directory the same as this script', file_type)
            buffer += 'pushd $DIR >/dev/null\n'
            buffer += _comment('get platform & architecture', file_type)
            buffer += 'PLAT="$(uname)"\n'
            buffer += 'ARCH="$(uname -m)"\n'

            buffer += 'PL_CONFIG=' + project._registered_configurations[0] + '\n'
            buffer += 'while getopts ":c:" option; do\n'
            buffer += '   case $option in\n'
            buffer += '   c) # set conf\n'
            buffer += '         PL_CONFIG=$OPTARG;;\n'
            buffer += '     \?) # Invalid option\n'
            buffer += '         echo "Error: Invalid option"\n'
            buffer += '         exit;;\n'
            buffer += '   esac\n'
            buffer += 'done\n'

            buffer += _new_lines(2)
            for register_config in project._registered_configurations:
                

                # find main target
                target_found = False        
                for target in project._targets:
                    if target._target_type == TargetType.EXECUTABLE and target._name == project._main_target_name:
                        for config in target._configurations:
                            if config._name == register_config:
                                for settings in config._compiler_settings:
                                    if settings._compiler_type == Compiler.GCC:
                                        buffer += 'if [[ "$PL_CONFIG" == "' + register_config + '" ]]; then\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=0\n'
                                        buffer += 'if lsof | grep -i -q ' + settings._output_binary + '\n'
                                        buffer += 'then\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=1\n'
                                        buffer += 'echo\n'
                                        buffer += 'echo ${BOLD}${WHITE}${RED_BG}--------${GREEN_BG} HOT RELOADING ${RED_BG}--------${NC}\n'
                                        buffer += 'echo\n'
                                        buffer += 'else\n'
                                        buffer += 'PL_HOT_RELOAD_STATUS=0\n'
                                        target_found = True
                for target in project._targets:
                    for config in target._configurations:
                        if config._name == register_config:
                            for settings in config._compiler_settings:
                                if settings._compiler_type == Compiler.GCC:
                                    if target._target_type == TargetType.EXECUTABLE:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                                    elif target._target_type == TargetType.DYNAMIC_LIBRARY:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + '_*' + settings._output_binary_extension + '\n'
                                    elif target._target_type == TargetType.STATIC_LIBRARY:
                                        buffer += 'rm -f ./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '\n'
                if target_found:
                    buffer += "fi\n"
                buffer += _new_lines(2)
                for target in project._targets:

                    for config in target._configurations:

                        if config._name == register_config:

                            for settings in config._compiler_settings:

                                if settings._compiler_type == Compiler.GCC:

                                    buffer += _title(config._name + " | " + target._name, file_type)
                                    buffer += _new_lines(1)
                                    buffer += _comment('create output directory', file_type)
                                    buffer += 'if ! [[ -d "' + settings._output_directory + '" ]]; then\n'
                                    buffer += '    mkdir "' + settings._output_directory + '"\n'
                                    buffer += 'fi\n'
                                    buffer += 'echo LOCKING > "./' + settings._output_directory + '/' + target._lock_file + '"'
                                    buffer += _new_lines(2)

                                    buffer += 'PL_DEFINES=\n'    
                                    for define in settings._definitions:
                                        buffer += 'PL_DEFINES+=" -D' + define + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_INCLUDE_DIRECTORIES=\n'    
                                    for include in settings._include_directories:
                                        buffer += 'PL_INCLUDE_DIRECTORIES+=" -I' + include + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINK_DIRECTORIES=\n'    
                                    for link in settings._link_directories:
                                        buffer += 'PL_LINK_DIRECTORIES+=" -L' + link + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_COMPILER_FLAGS=\n'    
                                    for flag in settings._compiler_flags:
                                        buffer += 'PL_COMPILER_FLAGS+=" ' + flag + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINKER_FLAGS=\n'    
                                    for flag in settings._linker_flags:
                                        buffer += 'PL_LINKER_FLAGS+=" -l' + flag + '"\n'
                                    buffer += _new_lines(1)

                                    buffer += 'PL_LINK_LIBRARIES=\n'    
                                    for link in settings._link_libraries:
                                        buffer += 'PL_LINK_LIBRARIES+=" -l' + link + '"\n'
                                    buffer += _new_lines(1)
                                    buffer += "PL_RESULT=${BOLD}${GREEN}Successful.${NC}"
                                    buffer += _new_lines(1)

                                    if target._target_type == TargetType.STATIC_LIBRARY:
        
                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling...${NC}\n'
                                    
                                        for source in settings._source_files:
                                            buffer += 'gcc -c -fPIC $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS ' + source + ' -o "./' + settings._output_directory + '/' + source + '.o"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

                                    elif target._target_type == TargetType.DYNAMIC_LIBRARY:

                                        buffer += 'PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += 'PL_SOURCES+=" ' + source + '"\n'
                                        buffer += _new_lines(1)

                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling and Linking...${NC}\n'
                                        buffer += 'gcc -shared -fPIC $PL_SOURCES $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS $PL_INCLUDE_DIRECTORIES $PL_LINK_DIRECTORIES $PL_LINKER_FLAGS $PL_LINK_LIBRARIES -o "./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension +'"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

                                    elif target._target_type == TargetType.EXECUTABLE:

                                        buffer += 'PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += 'PL_SOURCES+=" ' + source + '"\n'

                                        buffer += _new_lines(1)
                                        buffer += "echo\n"
                                        buffer += 'echo ${YELLOW}Step: ' + target._name +'${NC}\n'
                                        buffer += 'echo ${YELLOW}~~~~~~~~~~~~~~~~~~~${NC}\n'
                                        buffer += 'echo ${CYAN}Compiling and Linking...${NC}\n'
                                        buffer += 'gcc -fPIC $PL_SOURCES $PL_INCLUDE_DIRECTORIES $PL_DEFINES $PL_COMPILER_FLAGS $PL_INCLUDE_DIRECTORIES $PL_LINK_DIRECTORIES $PL_LINKER_FLAGS $PL_LINK_LIBRARIES -o "./' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension +'"\n'

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "if [ $? -ne 0 ]\n"
                                        buffer += "then\n"
                                        buffer += "    PL_RESULT=${BOLD}${RED}Failed.${NC}\n"
                                        buffer += "fi\n"
                                        buffer += "echo ${CYAN}Results: ${NC} ${PL_RESULT}\n"
                                        buffer += "echo ${CYAN}~~~~~~~~~~~~~~~~~~~~~~${NC}\n"
                                        buffer += 'rm "./' + settings._output_directory + '/' + target._lock_file + '"'
                                        buffer += _new_lines(2)

            buffer += 'fi\n'
            buffer += 'popd >/dev/null'
            file.write(buffer)

def generate_msvc_build():

    _setup_defaults()

    for project in _context._projects:

        filepath = project._working_directory + "/" + project._win32_script_name + ".bat"
        file_type = FileType.BATCH

        with open(filepath, "w") as file:
            buffer = ""
            buffer += _comment('do NOT keep changes to environment variables', file_type)
            buffer += '@setlocal'
            buffer += _new_lines(2)
            buffer += '@pushd %~dp0\n'
            buffer += '@set dir=%~dp0'
            buffer += _new_lines(2)
            buffer += _comment('setup development environment', file_type)
            buffer += '@set PL_RESULT=[1m[92mSuccessful.[0m\n'
            buffer += '@set PATH=C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build;%PATH%\n'
            buffer += '@set PATH=C:\\Program Files\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build;%PATH%\n'
            buffer += '@set PATH=C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build;%PATH%\n'
            buffer += '@set PATH=C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build;%PATH%\n'
            buffer += '@set PATH=C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise/VC\\Auxiliary\\Build;%PATH%\n'
            buffer += _new_lines(1)
            buffer += '@call vcvarsall.bat amd64 > nul'
            buffer += _new_lines(2)
            buffer += "@set PL_CONFIG=Debug\n"
            buffer += ":CheckOpts\n"
            buffer += '@if "%~1"=="-c" (@set PL_CONFIG=%2) & @shift & @shift & @goto CheckOpts'
            buffer += _new_lines(2)

            for register_config in project._registered_configurations:
                buffer += '@if "%PL_CONFIG%" equ "' + register_config +  '" ( goto ' + register_config + ' )\n'

            buffer += _new_lines(2)
            for register_config in project._registered_configurations:

                # find main target
                target_found = False
                for target in project._targets:
                    if target._target_type == TargetType.EXECUTABLE and target._name == project._main_target_name:
                        for config in target._configurations:
                            if config._name == register_config:
                                for settings in config._compiler_settings:
                                    if settings._compiler_type == Compiler.MSVC:
                                        buffer += ":" + register_config + "\n"
                                        buffer += "@set PL_HOT_RELOAD_STATUS=0\n"
                                        buffer += "@echo off\n"
                                        buffer += '2>nul (>>' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + ' echo off) && (@set PL_HOT_RELOAD_STATUS=0) || (@set PL_HOT_RELOAD_STATUS=1)\n'
                                        buffer += "@if %PL_HOT_RELOAD_STATUS% equ 1 (\n"
                                        buffer += "    @echo.\n"
                                        buffer += "    @echo [1m[97m[41m--------[42m HOT RELOADING [41m--------[0m\n"
                                        buffer += ")\n"
                                        buffer += _new_lines(2)
                                        buffer += "@if %PL_HOT_RELOAD_STATUS% equ 0 (\n    @echo.\n"
                                        target_found = True
                
                for target in project._targets:
                    for config in target._configurations:
                        if config._name == register_config:
                            for settings in config._compiler_settings:
                                if settings._compiler_type == Compiler.MSVC:
                                    buffer += '    @if exist "' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '"'
                                    buffer += ' del "' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '"'
                                    buffer += _new_lines(1)
                                if target._target_type == TargetType.DYNAMIC_LIBRARY:
                                    buffer += '    @if exist "' + settings._output_directory + '/' + settings._output_binary + '_*' + settings._output_binary_extension + '"'
                                    buffer += ' del "' + settings._output_directory + '/' + settings._output_binary + '_*' + settings._output_binary_extension + '"'
                                    buffer += _new_lines(1)
                                    buffer += '    @if exist "' + settings._output_directory + '/' + settings._output_binary + '_*.pdb"'
                                    buffer += ' del "' + settings._output_directory + '/' + settings._output_binary + '_*.pdb"'
                                    buffer += _new_lines(1)
                if target_found:
                    buffer += ")"
                buffer += _new_lines(2)
                for target in project._targets:

                    for config in target._configurations:

                        if config._name == register_config:

                            buffer += _title(config._name + " | " + target._name, file_type)
                            buffer += _new_lines(1)

                            for settings in config._compiler_settings:

                                if settings._compiler_type == Compiler.MSVC:

                                    buffer += _comment('create output directory', file_type)
                                    buffer += '@if not exist "' + settings._output_directory + '" @mkdir "' + settings._output_directory + '"'
                                    buffer += _new_lines(2)

                                    buffer += '@echo LOCKING > "' + settings._output_directory + '/' + target._lock_file + '"'
                                    buffer += _new_lines(2)
                                    
                                    buffer += '@set PL_DEFINES=\n'    
                                    for define in settings._definitions:
                                        buffer += '@set PL_DEFINES=-D' + define + " %PL_DEFINES%\n"
                                    buffer += _new_lines(1)

                                    buffer += '@set PL_INCLUDE_DIRECTORIES=\n'    
                                    for include in settings._include_directories:
                                        buffer += '@set PL_INCLUDE_DIRECTORIES=-I"' + include + '" %PL_INCLUDE_DIRECTORIES%\n'
                                    buffer += _new_lines(1)

                                    buffer += '@set PL_LINK_DIRECTORIES=\n'    
                                    for link in settings._link_directories:
                                        buffer += '@set PL_LINK_DIRECTORIES=-LIBPATH:"' + link + '" %PL_LINK_DIRECTORIES%\n'
                                    buffer += _new_lines(1)

                                    buffer += '@set PL_COMPILER_FLAGS=\n'    
                                    for flag in settings._compiler_flags:
                                        buffer += '@set PL_COMPILER_FLAGS=' + flag + " %PL_COMPILER_FLAGS%\n"
                                    buffer += _new_lines(1)

                                    buffer += '@set PL_LINKER_FLAGS=\n'    
                                    for flag in settings._linker_flags:
                                        buffer += '@set PL_LINKER_FLAGS=' + flag + " %PL_LINKER_FLAGS%\n"
                                    buffer += _new_lines(1)

                                    buffer += '@set PL_LINK_LIBRARIES=\n'    
                                    for link in settings._link_libraries:
                                        buffer += '@set PL_LINK_LIBRARIES=' + link + " %PL_LINK_LIBRARIES%\n"
                                    buffer += _new_lines(1)

                                    if target._target_type == TargetType.STATIC_LIBRARY:
        
                                        buffer += _comment('run compiler', file_type)
                                        buffer += "@echo.\n"
                                        buffer += '@echo [1m[93mStep: ' + target._name +'[0m\n'
                                        buffer += '@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m\n'
                                        buffer += '@echo [1m[36mCompiling...[0m\n'
                                    
                                        for source in settings._source_files:
                                            buffer += 'cl -c %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% ' + source + ' -Fe"' + settings._output_directory + '/' + source + '.obj" -Fo"' + settings._output_directory + '/"'
                                            buffer += _new_lines(1)

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "@set PL_BUILD_STATUS=%ERRORLEVEL%\n"
                                        buffer += "@if %PL_BUILD_STATUS% NEQ 0 (\n"
                                        buffer += "    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%\n"
                                        buffer += "    @set PL_RESULT=[1m[91mFailed.[0m\n"
                                        buffer += "    goto " + 'Cleanup' + target._name
                                        buffer += ")\n"

                                        buffer += _new_lines(1)
                                        buffer += _comment('link object files into a shared lib', file_type)
                                        buffer += "@echo [1m[36mLinking...[0m\n"
                                        buffer += 'lib -nologo -OUT:"' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '" "' + settings._output_directory + '/*.obj"'
                                        buffer += _new_lines(2)
                                        buffer += ':Cleanup' + target._name
                                        buffer += '\n    @echo [1m[36mCleaning...[0m\n'
                                        buffer += '    @del "' + settings._output_directory + '/*.obj"  > nul 2> nul'

                                    elif target._target_type == TargetType.DYNAMIC_LIBRARY:

                                        buffer += '@set PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += '@set PL_SOURCES="' + source + '" %PL_SOURCES%\n'
                                        buffer += _new_lines(1)

                                        buffer += _comment('run compiler', file_type)
                                        buffer += "@echo.\n"
                                        buffer += '@echo [1m[93mStep: ' + target._name +'[0m\n'
                                        buffer += '@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m\n'
                                        buffer += '@echo [1m[36mCompiling and Linking...[0m\n'
                                        buffer += 'cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '" -Fo"' + settings._output_directory + '/" -LD -link %PL_LINKER_FLAGS% -PDB:"' + settings._output_directory + '/' + settings._output_binary + '_%random%.pdb"' + ' %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%'
                                        buffer += _new_lines(1)

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "@set PL_BUILD_STATUS=%ERRORLEVEL%\n"
                                        buffer += "@if %PL_BUILD_STATUS% NEQ 0 (\n"
                                        buffer += "    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%\n"
                                        buffer += "    @set PL_RESULT=[1m[91mFailed.[0m\n"
                                        buffer += "    goto " + 'Cleanup' + target._name
                                        buffer += ")\n"

                                        buffer += _new_lines(2)
                                        buffer += ':Cleanup' + target._name
                                        buffer += '\n    @echo [1m[36mCleaning...[0m\n'
                                        buffer += '    @del "' + settings._output_directory + '/*.obj"  > nul 2> nul'

                                    elif target._target_type == TargetType.EXECUTABLE:

                                        
                                        buffer += '@set PL_SOURCES=\n'    
                                        for source in settings._source_files:
                                            buffer += '@set PL_SOURCES="' + source + '" %PL_SOURCES%\n'
                                        buffer += _new_lines(1)

                                        buffer += '@if %PL_HOT_RELOAD_STATUS% equ 1 ( goto ' + 'Cleanup' + target._name + ' )\n'   
                                        buffer += _comment('run compiler', file_type)
                                        buffer += "@echo.\n"
                                        buffer += '@echo [1m[93mStep: ' + target._name +'[0m\n'
                                        buffer += '@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m\n'
                                        buffer += '@echo [1m[36mCompiling and Linking...[0m\n'
                                        buffer += 'cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"' + settings._output_directory + '/' + settings._output_binary + settings._output_binary_extension + '" -Fo"' + settings._output_directory + '/" -link %PL_LINKER_FLAGS% %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%'
                                        buffer += _new_lines(1)

                                        buffer += _new_lines(1)
                                        buffer += _comment("check build status", file_type)
                                        buffer += "@set PL_BUILD_STATUS=%ERRORLEVEL%\n"
                                        buffer += "@if %PL_BUILD_STATUS% NEQ 0 (\n"
                                        buffer += "    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%\n"
                                        buffer += "    @set PL_RESULT=[1m[91mFailed.[0m\n"
                                        buffer += "    goto " + 'Cleanup' + target._name
                                        buffer += "\n)\n"

                                        buffer += _new_lines(2)
                                        buffer += ':Cleanup' + target._name
                                        buffer += '\n    @echo [1m[36mCleaning...[0m\n'
                                        buffer += '    @del "' + settings._output_directory + '/*.obj"  > nul 2> nul\n'

                                    buffer += _new_lines(2)
                                    buffer += '@del "' + settings._output_directory + '/' + target._lock_file + '"'
                                    buffer += _new_lines(2)

                                    buffer += "@echo.\n"
                                    buffer += "@echo [36mResult: [0m %PL_RESULT%\n"
                                    buffer += "@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m\n"
                                    buffer += _new_lines(1)
                if target_found:
                    buffer += "goto ExitLabel\n"

            buffer += ':ExitLabel\n'
            buffer += '@popd'
            file.write(buffer)

