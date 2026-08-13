add_executable(posterwallutils_test
    posterwallutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore/utils/mediapaginationutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/posterwallutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/posterstageresponsiveutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/seasonresponsiveutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/modelnotificationutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/mediaimagecandidateutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/librarypaginationutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/dialogresponsiveutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/dashboardrequestlimitutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/dashboardresponsiveutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/xplayerresponsiveutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/gallerylayoututils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/buttonlayoututils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/mediacardlayoututils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playbackengineprofile.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playbackloadingstateutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playbackwindowmodeutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playercontroltextutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playerhudvisibilityutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playermediaswitchertextutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playerpopupanimationutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playerpopupstateutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playertrackoptiontextutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/windowsmaterial.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/posterstagewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/settingssubpanel.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/slidingstackedwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/managers/thememanager.cpp
)

target_include_directories(posterwallutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(posterwallutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME posterwallutils_test COMMAND posterwallutils_test)

add_executable(posterwallsnapshotstore_test
    posterwallsnapshotstore_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/posterwallsnapshotstore.cpp
)

target_include_directories(posterwallsnapshotstore_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(posterwallsnapshotstore_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME posterwallsnapshotstore_test COMMAND posterwallsnapshotstore_test)

add_executable(posterwallpreparationpolicy_test
    posterwallpreparationpolicy_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/posterwallpreparationpolicy.cpp
)

target_include_directories(posterwallpreparationpolicy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(posterwallpreparationpolicy_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(posterwallpreparationpolicy_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME posterwallpreparationpolicy_test
         COMMAND posterwallpreparationpolicy_test)

add_executable(layoutgeometryutils_test
    layoutgeometryutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/buttonlayoututils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/mediacardlayoututils.cpp
)

target_include_directories(layoutgeometryutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(layoutgeometryutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME layoutgeometryutils_test COMMAND layoutgeometryutils_test)

add_executable(widgetmotionutils_test
    widgetmotionutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/widgetgeometryutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/widgetmotion.cpp
)

target_include_directories(widgetmotionutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(widgetmotionutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME widgetmotionutils_test COMMAND widgetmotionutils_test)

add_executable(motioncomponents_test
    motioncomponents_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/collapsiblesection.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/playerlongpresshandler.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/moderntoast.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/playerosdlayer.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/textwraputils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/widgetgeometryutils.cpp
)

target_include_directories(motioncomponents_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(motioncomponents_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME motioncomponents_test COMMAND motioncomponents_test)

add_executable(medialistmodel_test
    medialistmodel_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/views/media/medialistmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/mediaimagecandidateutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/modelnotificationutils.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/pixmapcachebudgetutils.cpp
)

target_include_directories(medialistmodel_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(medialistmodel_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME medialistmodel_test COMMAND medialistmodel_test)

add_executable(searchhistorypopup_test
    searchhistorypopup_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/searchhistorypopup.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/searchhistorychip.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/components/flowlayout.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/managers/thememanager.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/resources/resources.qrc
)

target_include_directories(searchhistorypopup_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(searchhistorypopup_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME searchhistorypopup_test COMMAND searchhistorypopup_test)

add_executable(taskssnapshotutils_test
    taskssnapshotutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/taskssnapshotutils.cpp
)

target_include_directories(taskssnapshotutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(taskssnapshotutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME taskssnapshotutils_test COMMAND taskssnapshotutils_test)

add_executable(libraryorderutils_test
    libraryorderutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/libraryorderutils.cpp
)

target_include_directories(libraryorderutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(libraryorderutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME libraryorderutils_test COMMAND libraryorderutils_test)

add_executable(imageconcurrencyutils_test
    imageconcurrencyutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore/utils/imageconcurrencyutils.cpp
)

target_include_directories(imageconcurrencyutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(imageconcurrencyutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME imageconcurrencyutils_test COMMAND imageconcurrencyutils_test)

add_executable(smoothscrollcontroller_test
    smoothscrollcontroller_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/smoothscrollcontroller.cpp
)

target_include_directories(smoothscrollcontroller_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(smoothscrollcontroller_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME smoothscrollcontroller_test COMMAND smoothscrollcontroller_test)

add_executable(dashboardloadstageutils_test
    dashboardloadstageutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/dashboardloadstageutils.cpp
)

target_include_directories(dashboardloadstageutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(dashboardloadstageutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(dashboardloadstageutils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME dashboardloadstageutils_test COMMAND dashboardloadstageutils_test)

add_executable(imagecachebudgetutils_test
    imagecachebudgetutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore/utils/imagecachebudgetutils.cpp
)

target_include_directories(imagecachebudgetutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(imagecachebudgetutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(imagecachebudgetutils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME imagecachebudgetutils_test COMMAND imagecachebudgetutils_test)

add_executable(pixmapcachebudgetutils_test
    pixmapcachebudgetutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/pixmapcachebudgetutils.cpp
)

target_include_directories(pixmapcachebudgetutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(pixmapcachebudgetutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(pixmapcachebudgetutils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME pixmapcachebudgetutils_test COMMAND pixmapcachebudgetutils_test)

add_executable(favoritesrefreshpolicy_test
    favoritesrefreshpolicy_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/favoritesrefreshpolicy.cpp
)

target_include_directories(favoritesrefreshpolicy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(favoritesrefreshpolicy_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(favoritesrefreshpolicy_test PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME favoritesrefreshpolicy_test COMMAND favoritesrefreshpolicy_test)

add_executable(playbackprogresspolicy_test
    playbackprogresspolicy_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playbackprogresspolicy.cpp
)

target_include_directories(playbackprogresspolicy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(playbackprogresspolicy_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(playbackprogresspolicy_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME playbackprogresspolicy_test COMMAND playbackprogresspolicy_test)

add_executable(playerinpututils_test
    playerinpututils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playerinpututils.cpp
)

target_include_directories(playerinpututils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(playerinpututils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(playerinpututils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME playerinpututils_test COMMAND playerinpututils_test)

add_executable(playerwindowgeometryutils_test
    playerwindowgeometryutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/playerwindowgeometryutils.cpp
)

target_include_directories(playerwindowgeometryutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(playerwindowgeometryutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(playerwindowgeometryutils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME playerwindowgeometryutils_test COMMAND playerwindowgeometryutils_test)

add_executable(diskcachemaintenanceutils_test
    diskcachemaintenanceutils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/diskcachemaintenanceutils.cpp
)

target_include_directories(diskcachemaintenanceutils_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(diskcachemaintenanceutils_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

set_target_properties(diskcachemaintenanceutils_test PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_test(NAME diskcachemaintenanceutils_test COMMAND diskcachemaintenanceutils_test)

add_executable(singleflightrefreshpolicy_test
    singleflightrefreshpolicy_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/utils/singleflightrefreshpolicy.cpp
)

target_include_directories(singleflightrefreshpolicy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
)

target_link_libraries(singleflightrefreshpolicy_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME singleflightrefreshpolicy_test COMMAND singleflightrefreshpolicy_test)

add_executable(configstore_deferredsync_test
    configstore_deferredsync_test.cpp
)

target_include_directories(configstore_deferredsync_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(configstore_deferredsync_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME configstore_deferredsync_test COMMAND configstore_deferredsync_test)

add_executable(singleapplicationmanager_test
    singleapplicationmanager_test.cpp
    ${CMAKE_SOURCE_DIR}/src/XplayerApp/managers/singleapplicationmanager.cpp
)

target_include_directories(singleapplicationmanager_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerApp
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)

target_link_libraries(singleapplicationmanager_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Test
    XplayerCore
)

add_test(NAME singleapplicationmanager_test COMMAND singleapplicationmanager_test)

add_executable(playedhistory_probe playedhistory_probe.cpp)
target_include_directories(playedhistory_probe PRIVATE
    ${CMAKE_SOURCE_DIR}/src/XplayerCore
)
target_link_libraries(playedhistory_probe PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    XplayerCore
)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(xplayer_mingw_bin
        "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(xplayer_qt_bin
        "${Qt6Core_DIR}/../../../bin" ABSOLUTE)
    set(xplayer_test_path
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY};${xplayer_qt_bin};${xplayer_mingw_bin};$ENV{PATH}")
    string(REPLACE ";" "\\;" xplayer_test_path "${xplayer_test_path}")
    get_property(xplayer_test_names DIRECTORY PROPERTY TESTS)
    foreach(xplayer_test_name IN LISTS xplayer_test_names)
        set_tests_properties("${xplayer_test_name}"
            PROPERTIES ENVIRONMENT "PATH=${xplayer_test_path}")
    endforeach()
endif()
