cmake_minimum_required(VERSION 3.15)

# 强制使用 Visual Studio 的 CMake -----------
if(WIN32)
    # 尝试查找 Visual Studio 的 CMake
    find_program(VS_CMAKE
        NAMES cmake
        PATHS 
            "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
            "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
            "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
        NO_DEFAULT_PATH
    )
    
    if(VS_CMAKE)
        # 覆盖默认的 CMake 命令
        set(CMAKE_COMMAND "${VS_CMAKE}" CACHE STRING "CMake command" FORCE)
    else()
        message(WARNING "Visual Studio CMake not found. Using system CMake: ${CMAKE_COMMAND}")
    endif()
endif()


project(DesktopPet LANGUAGES CXX)

# 禁止源码内构建
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Prohibit building in the source code directory! Please use the build folder"
        
    )
endif()

# 显示使用的 CMake 路径
message(STATUS "******************************************************")
message(STATUS "Using CMake from: ${CMAKE_COMMAND}")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "******************************************************")

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------
# 平台特定配置（VS2022）
# --------------------------
if(MSVC)
    # 设置默认生成x64平台
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Target platform" FORCE)
    endif()
    
    # 设置默认构建类型为Release
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
endif()

# 源码和头文件
file(GLOB_RECURSE DESKPET_SRC
    "${CMAKE_SOURCE_DIR}/DesktopPet/*.cpp"
    "${CMAKE_SOURCE_DIR}/DesktopPet/*.h"
)

# 资源文件
set(DESKPET_RC "${CMAKE_SOURCE_DIR}/DesktopPet/Resource.rc")
set(DESKPET_ICON "${CMAKE_SOURCE_DIR}/DesktopPet/main_icon.ico")

# 添加可执行文件 - 设置为WIN32应用程序而非控制台
add_executable(DesktopPet WIN32 ${DESKPET_SRC} ${DESKPET_RC})

# --------------------------
# 所有目标操作必须在此之后
# --------------------------

# Windows资源文件支持
if(MSVC)
    set_source_files_properties(${DESKPET_RC} PROPERTIES LANGUAGE RC)
endif()

# 在所有 MSVC 编译器中设置 Unicode 字符集属性
if(MSVC)
    # 设置高级属性中的字符集为 Unicode
    set_target_properties(DesktopPet PROPERTIES 
        VS_GLOBAL_CharacterSet "Unicode"  # 这会直接影响高级选项卡中的设置
    )
    
    # 同时添加预处理器定义以确保编译一致性
    target_compile_definitions(DesktopPet PRIVATE 
        UNICODE
        _UNICODE
    )
endif()

# 头文件包含目录
target_include_directories(DesktopPet PRIVATE
    "${CMAKE_SOURCE_DIR}/DesktopPet/include"  # JsonCpp头文件
    "${CMAKE_SOURCE_DIR}/DesktopPet"          # 项目自身头文件
)

# 链接库目录
target_link_directories(DesktopPet PRIVATE
    "${CMAKE_SOURCE_DIR}/DesktopPet/lib"      # 第三方库目录
)

# 链接Windows库
if(MSVC)
    target_link_libraries(DesktopPet PRIVATE
        gdiplus
        comctl32
        shlwapi
        psapi
        pdh
        iphlpapi
        jsoncpp
    )
endif()

# 输出目录设置
set(BIN_BASE_DIR "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(DesktopPet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${BIN_BASE_DIR}/${CMAKE_CFG_INTDIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${BIN_BASE_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BIN_BASE_DIR}/Release"
    WIN32_EXECUTABLE TRUE  # 确保是窗口应用程序
)

# 强制使用Windows子系统
if(MSVC)
    target_link_options(DesktopPet PRIVATE 
        "/SUBSYSTEM:WINDOWS"
        "/ENTRY:WinMainCRTStartup"  # ANSI 入口点
    )
endif()

# 添加自定义变量确保使用正确的 CMake 路径
set(CMAKE_COMMAND_SAVE "${CMAKE_COMMAND}")


# 资源文件和图标分组
source_group("Resource Files" FILES ${DESKPET_RC} ${DESKPET_ICON})

# 头文件分组
file(GLOB DESKPET_HEADERS 
    "${CMAKE_SOURCE_DIR}/DesktopPet/*.h"
    "${CMAKE_SOURCE_DIR}/DesktopPet/*.hpp"
)
source_group("Header Files" FILES ${DESKPET_HEADERS})

# JsonCpp头文件分组
file(GLOB JSONCPP_HEADERS "${CMAKE_SOURCE_DIR}/DesktopPet/include/json/*.h")
source_group("JsonCpp Headers" FILES ${JSONCPP_HEADERS})

# 确保所有配置都包含Unicode设置
set_property(TARGET DesktopPet PROPERTY VS_GLOBAL_CharacterSet "Unicode")