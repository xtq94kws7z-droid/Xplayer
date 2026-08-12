# src/XplayerCore/CMakeLists.txt

file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(XplayerCore SHARED
    ${CORE_HEADERS}
    ${CORE_SOURCES}
)

target_include_directories(XplayerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(XplayerCore PUBLIC Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::WebSockets
    QCoro6::Core
    QCoro6::Network
)

target_compile_definitions(XplayerCore PRIVATE
    XPLAYERCORE_LIBRARY
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:QT_NO_DEBUG_OUTPUT;QT_NO_INFO_OUTPUT>"
)

include(GNUInstallDirs)

set(_xplayer_core_library_install_dir "${CMAKE_INSTALL_LIBDIR}")
if(UNIX AND NOT APPLE AND XPLAYER_RUNTIME_LIB_SUBDIR)
    set(_xplayer_core_library_install_dir
        "${CMAKE_INSTALL_LIBDIR}/${XPLAYER_RUNTIME_LIB_SUBDIR}")
endif()

install(TARGETS XplayerCore
    LIBRARY DESTINATION ${_xplayer_core_library_install_dir}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${_xplayer_core_library_install_dir}
)
