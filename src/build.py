import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/../tools")

import pl_build as pl

with pl.project("pilotlight") as pilotlight:
    pl.add_configuration("debug")
    pl.add_configuration("debugdx11")
    pl.set_working_directory(".")
    pl.set_main_target("pilot_light")
    pl.set_build_win32_script_name("build")
    pl.set_build_macos_script_name("build_macos")
    pl.set_build_linux_script_name("build_linux")

    def msvc_project_commons():
        pl.add_include_directories('%WindowsSdkDir%Include\\um', '%WindowsSdkDir%Include\\shared', '%DXSDK_DIR%Include', "%VULKAN_SDK%\\Include")
        pl.add_compiler_flags("-Zc:preprocessor", "-nologo", "-std:c11", "-W4", "-WX", "-wd4201", "-wd4100", "-wd4996", "-wd4505", "-wd4189", "-wd5105", "-wd4115", "-permissive-")
        pl.add_definition("PL_VULKAN_BACKEND")
        pl.add_link_directory('%VULKAN_SDK%\\Lib')

    def gcc_project_commons():
        pl.add_include_directory('$VULKAN_SDK/include')
        pl.add_include_directory('/usr/include/vulkan')
        pl.add_link_directories('$VULKAN_SDK/lib', "/usr/lib/x86_64-linux-gnu")
        pl.add_link_libraries("vulkan", "xcb", "X11", "X11-xcb", "xkbcommon")
        pl.add_compiler_flag("-std=gnu99")
        pl.add_compiler_flags("--debug", "-g")
        pl.add_linker_flags("dl", "m")
        pl.add_definition("PL_VULKAN_BACKEND")

    def clang_project_commons():
        pl.add_definition("PL_METAL_BACKEND")
        pl.add_compiler_flags("-std=c99", "-fmodules", "-ObjC", "--debug", "-g")
        pl.add_link_libraries("Metal", "MetalKit", "Cocoa", "IOKit", "CoreVideo", "QuartzCore")

    def project_commons():
        pl.add_include_directories("../out", "../dependencies/stb", "../src", "../extensions")
        pl.set_output_directory("../out")
        pl.add_link_directory("../out")
        pl.add_definitions("_USE_MATH_DEFINES", "PL_PROFILING_ON", "PL_ALLOW_HOT_RELOAD", "PL_ENABLE_VALIDATION_LAYERS")
        
    ###############################################################################
    #                                 pl_lib                                      #
    ###############################################################################
    with pl.target("pl_lib", pl.TargetType.STATIC_LIBRARY):

        def config_commons():
            pl.set_output_binary("pilotlight")
            pl.add_source_file("pilotlight.c")

        with pl.configuration("debug"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_project_commons()
                project_commons()
                config_commons()
                pl.add_definition("_DEBUG")  
                pl.add_compiler_flags("-Od", "-MDd", "-Zi")
            with pl.compiler("gcc", pl.Compiler.GCC):
                project_commons()
                config_commons()
                gcc_project_commons()
            with pl.compiler("clang", pl.Compiler.CLANG):
                project_commons()
                config_commons()
                clang_project_commons()

        with pl.configuration("debugdx11"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_project_commons()
                project_commons()
                config_commons()
                pl.add_compiler_flags("-O2", "-MD")

    ###############################################################################
    #                                 pl_draw_extension                           #
    ###############################################################################
    with pl.target("pl_draw_extension", pl.TargetType.DYNAMIC_LIBRARY):
        
        def msvc_config_commons():
            pl.add_linker_flags("-noimplib", "-noexp", "-incremental:no")
            pl.add_link_library("pilotlight.lib")
            pl.add_compiler_flags("-Od", "-MDd", "-Zi")

        def config_commons():
            pl.set_output_binary("pl_draw_extension")
            pl.add_source_file("../extensions/pl_draw_extension.c")
            pl.add_definition("_DEBUG") 
            
        with pl.configuration("debug"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_config_commons()
                msvc_project_commons()
                project_commons()
                config_commons()
                
            with pl.compiler("gcc", pl.Compiler.GCC):
                project_commons()
                gcc_project_commons()
                config_commons()
                pl.add_source_file("../out/pilotlight.c.o")
            with pl.compiler("clang", pl.Compiler.CLANG):
                project_commons()
                config_commons()
                clang_project_commons()
                pl.add_source_file("../out/pilotlight.c.o")

        with pl.configuration("debugdx11"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_config_commons()
                msvc_project_commons()
                project_commons()
                config_commons()
                pl.add_compiler_flags("-O2", "-MD")

    ###############################################################################
    #                                    app                                      #
    ###############################################################################
    with pl.target("app", pl.TargetType.DYNAMIC_LIBRARY):

        def msvc_config_commons():
            pl.add_linker_flags("-noimplib", "-noexp", "-incremental:no")
            pl.add_link_library("pilotlight.lib")
            pl.add_compiler_flags("-Od", "-MDd", "-Zi")
        
        def config_commons():
            pl.set_output_binary("app")
            
        with pl.configuration("debug"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_config_commons()
                msvc_project_commons()
                project_commons()
                config_commons()
                pl.add_source_file("app_vulkan.c")
            with pl.compiler("gcc", pl.Compiler.GCC):
                project_commons()
                gcc_project_commons()
                config_commons()
                pl.add_source_file("app_vulkan.c")
                pl.add_source_file("../out/pilotlight.c.o")
            with pl.compiler("clang", pl.Compiler.CLANG):
                project_commons()
                config_commons()
                clang_project_commons()
                pl.add_source_file("app_metal.m")
                pl.add_source_file("../out/pilotlight.c.o")

        with pl.configuration("debugdx11"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_project_commons()
                msvc_config_commons()
                project_commons()
                config_commons()
                pl.add_source_file("app_dx11.c")

    ###############################################################################
    #                                 pilot_light                                 #
    ###############################################################################
    with pl.target("pilot_light", pl.TargetType.EXECUTABLE):

        def msvc_config_commons():
            pl.add_linker_flags("-noimplib", "-noexp", "-incremental:no")
            pl.add_link_library("pilotlight.lib")
            pl.add_source_file("pl_main_win32.c")
            pl.add_compiler_flags("-Od", "-MDd", "-Zi")

        def config_commons():
            pl.set_output_binary("pilot_light")
               
        with pl.configuration("debug"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_project_commons()
                msvc_config_commons()
                project_commons()
                config_commons()
                pl.add_definition("_DEBUG")
            with pl.compiler("gcc", pl.Compiler.GCC):
                project_commons()
                gcc_project_commons()
                config_commons()
                pl.add_source_file("../out/pilotlight.c.o")
                pl.add_source_file("pl_main_linux.c")
            with pl.compiler("clang", pl.Compiler.CLANG):
                project_commons()
                config_commons()
                clang_project_commons()
                pl.add_source_file("../out/pilotlight.c.o")
                pl.add_source_file("pl_main_macos.m")

        with pl.configuration("debugdx11"):
            with pl.compiler("msvc", pl.Compiler.MSVC):
                msvc_project_commons()
                msvc_config_commons()
                project_commons()
                config_commons()
           
pl.generate_msvc_build()
pl.generate_linux_build()
pl.generate_macos_build()