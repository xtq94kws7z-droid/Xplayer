function(xplayer_append_default_qt_prefixes)
    if(XPLAYER_QT_ROOT)
        list(APPEND CMAKE_PREFIX_PATH "${XPLAYER_QT_ROOT}")
    endif()

    if(DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    endif()

    if(WIN32)
        set(_xplayer_qt_default_prefixes
            "C:/Qt/6.9.2/msvc2022_64"
            "C:/Qt/6.8.3/msvc2022_64"
            "C:/Qt/6.7.3/msvc2022_64"
            "E:/Qt6/6.9.2/msvc2022_64"
            "E:/Qt/6.9.2/msvc2022_64"
        )

        foreach(_prefix IN LISTS _xplayer_qt_default_prefixes)
            if(EXISTS "${_prefix}")
                list(APPEND CMAKE_PREFIX_PATH "${_prefix}")
            endif()
        endforeach()
    endif()

    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
endfunction()

function(xplayer_link_libmpv target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "xplayer_link_libmpv called with unknown target: ${target_name}")
    endif()

    set(_libmpv_root "${CMAKE_SOURCE_DIR}/libs/libmpv")
    set(_libmpv_include "${_libmpv_root}/include")

    if(WIN32)
        find_library(MPV_LIBRARY
            NAMES mpv libmpv mpv-2 libmpv-2
            PATHS
                "${_libmpv_root}/lib"
                "${_libmpv_root}/lib/x64"
                "${_libmpv_root}/bin"
            NO_DEFAULT_PATH
        )

        if(NOT EXISTS "${_libmpv_include}/mpv/client.h" OR NOT MPV_LIBRARY)
            message(FATAL_ERROR
                "libmpv SDK is required. Place it under libs/libmpv with "
                "include/mpv/client.h and lib/libmpv.dll.a or lib/mpv.lib. "
                "See libs/libmpv/README.md if present.")
        endif()

        target_include_directories(${target_name} PRIVATE "${_libmpv_include}")
        target_link_libraries(${target_name} PRIVATE "${MPV_LIBRARY}")

        find_file(MPV_RUNTIME_DLL
            NAMES mpv-2.dll libmpv-2.dll mpv.dll libmpv.dll
            PATHS "${_libmpv_root}/bin" "${_libmpv_root}"
            NO_DEFAULT_PATH
        )

        if(MPV_RUNTIME_DLL)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MPV_RUNTIME_DLL}"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Copying libmpv runtime DLL")
        endif()
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
        target_link_libraries(${target_name} PRIVATE PkgConfig::MPV)
    endif()
endfunction()
