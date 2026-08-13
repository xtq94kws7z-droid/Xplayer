cmake_minimum_required(VERSION 3.19)

project(Xplayer VERSION 1.0.7 LANGUAGES CXX)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(XplayerPlatformDeps)

set(XPLAYER_RUNTIME_LIB_SUBDIR "xplayer" CACHE STRING
    "Subdirectory under the install libdir for bundled private runtime libraries")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/MP)
endif()

# ==========================================
# 允许通过缓存变量或环境变量提供 Qt 安装路径
# Windows 未显式指定时仍会尝试兼容现有本地默认路径
# ==========================================
set(XPLAYER_QT_ROOT "" CACHE PATH
    "Optional Qt installation root or CMake prefix path")
xplayer_append_default_qt_prefixes()

# 寻找 Qt6 (不再允许降级)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network Concurrent OpenGLWidgets LinguistTools WebSockets Test)
if(UNIX AND NOT APPLE)
    find_package(Qt6 QUIET COMPONENTS DBus)
endif()

# ==========================================
# 为第三方子模块 (qwindowkit/qmsetup) 注入所需的 Qt 版本环境变量
# ==========================================
set(QT_VERSION_MAJOR ${Qt6_VERSION_MAJOR})
set(QT_VERSION_MINOR ${Qt6_VERSION_MINOR})
set(QT_VERSION ${Qt6_VERSION})

# ==========================================
# 引入 QCoro (C++20 Coroutines for Qt)
# ==========================================
include(FetchContent)

function(xplayer_ensure_noop_install_script build_dir)
    if(NOT build_dir)
        return()
    endif()

    file(MAKE_DIRECTORY "${build_dir}")
    file(WRITE "${build_dir}/cmake_install.cmake"
        "# Xplayer placeholder install script for third-party dependency\n")
endfunction()

set(_xplayer_saved_skip_install_rules ${CMAKE_SKIP_INSTALL_RULES})
set(CMAKE_SKIP_INSTALL_RULES ON)
set(XPLAYER_QCORO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qcoro" CACHE PATH
    "Optional local QCoro source directory")
if(EXISTS "${XPLAYER_QCORO_SOURCE_DIR}/CMakeLists.txt")
    FetchContent_Declare(
        QCoro
        SOURCE_DIR "${XPLAYER_QCORO_SOURCE_DIR}"
    )
else()
    FetchContent_Declare(
        QCoro
        GIT_REPOSITORY https://github.com/danvratil/qcoro.git
        GIT_TAG v0.10.0
    )
endif()

# 强制 QCoro 使用 Qt6 进行编译，并关闭不必要的测试与示例构建以加快编译速度
set(QCORO_QT_MAJOR_VERSION 6 CACHE STRING "Use Qt6 for QCoro" FORCE)
set(QCORO_BUILD_EXAMPLES OFF CACHE BOOL "Disable QCoro examples" FORCE)
set(QCORO_WITH_QML OFF CACHE BOOL "Disable QCoro QML module" FORCE)
set(QCORO_WITH_QTQUICK OFF CACHE BOOL "Disable QCoro QtQuick module" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable QCoro tests" FORCE)

FetchContent_MakeAvailable(QCoro)
FetchContent_GetProperties(QCoro)
xplayer_ensure_noop_install_script("${qcoro_BINARY_DIR}")

# ==========================================
# 引入 spdlog (高性能线程安全日志库, MIT License)
# ==========================================
set(XPLAYER_SPDLOG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog" CACHE PATH
    "Optional local spdlog source directory")
if(EXISTS "${XPLAYER_SPDLOG_SOURCE_DIR}/CMakeLists.txt")
    FetchContent_Declare(
        spdlog
        SOURCE_DIR "${XPLAYER_SPDLOG_SOURCE_DIR}"
    )
else()
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )
endif()
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Disable spdlog examples" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Disable spdlog tests" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "Disable spdlog install rules" FORCE)
FetchContent_MakeAvailable(spdlog)
FetchContent_GetProperties(spdlog)
xplayer_ensure_noop_install_script("${spdlog_BINARY_DIR}")
set(CMAKE_SKIP_INSTALL_RULES ${_xplayer_saved_skip_install_rules})
unset(_xplayer_saved_skip_install_rules)
# ==========================================

set(QWINDOWKIT_INSTALL OFF CACHE BOOL
    "Disable QWindowKit development install rules inside Xplayer" FORCE)

add_subdirectory(libs/qwindowkit)
add_subdirectory(libs/qwindowkit/examples/shared)
add_subdirectory(src/XplayerCore)
add_subdirectory(src/XplayerApp)
enable_testing()
add_subdirectory(tests)

include(XplayerPackaging)
