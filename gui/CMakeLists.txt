
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
	enable_language(OBJCXX)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Concurrent Svg Qml Quick)
find_package(Qt6 COMPONENTS DBus)

if(WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

set(SOURCE_FILES
	include/exception.h
	src/main.cpp
	include/discoverymanager.h
	src/discoverymanager.cpp
	include/streamsession.h
	src/streamsession.cpp
	include/sessionlog.h
	src/sessionlog.cpp
	include/settings.h
	src/settings.cpp
	include/host.h
	src/host.cpp
	res/resources.qrc
	include/controllermanager.h
	src/controllermanager.cpp
	include/psnaccountid.h
	src/psnaccountid.cpp
	include/psntoken.h
	src/psntoken.cpp
	include/jsonrequester.h
	src/jsonrequester.cpp
	src/qml/qml.qrc
	include/qmlmainwindow.h
	src/qmlmainwindow.cpp
	include/qmlbackend.h
	src/qmlbackend.cpp
	include/qmlcontroller.h
	src/qmlcontroller.cpp
	include/qmlsettings.h
	src/qmlsettings.cpp
	include/qmlsvgprovider.h
	src/qmlsvgprovider.cpp
	include/systemdinhibit.h
	src/systemdinhibit.cpp
        include/qmlopenglmainwindow.h
        src/qmlopenglmainwindow.cpp
	)
set(RESOURCE_FILES "")

if(APPLE)
	list(APPEND RESOURCE_FILES "chiaki4deck.icns")
elseif(WIN32)
	list(APPEND RESOURCE_FILES "chiaki4deck.rc")
endif()

if(CHIAKI_ENABLE_STEAMDECK_NATIVE AND LINUX)
	list(APPEND SOURCE_FILES
		include/sdinputcontext.h
		src/sdinputcontext.cpp
	)
endif()

if(APPLE)
	list(APPEND SOURCE_FILES
		include/macMicPermission.h
		src/macMicPermission.m
	)
endif()

if (ANDROID)



        #add_library(chiaki SHARED
         #               ${RESOURCE_FILES}
         #               ${SOURCE_FILES}
        #)

        #set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
        qt_add_executable(chiaki  ${RESOURCE_FILES} ${SOURCE_FILES}
                                )
        set_property(TARGET chiaki APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                         ${CMAKE_CURRENT_SOURCE_DIR}/android)


else ()
	#qt_add_executable(chiaki  ${RESOURCE_FILES} ${SOURCE_FILES}
	#)
	add_executable(chiaki WIN32
			${RESOURCE_FILES}
			${SOURCE_FILES}
	)
endif ()
target_include_directories(chiaki PRIVATE include)

add_library(placebo-libav-impl src/libav_impl.c)
if (ANDROID)
	pkg_search_module(LIBPLACEBO REQUIRED libplacebo IMPORTED_TARGET)
	pkg_search_module(LIBAVCODEC REQUIRED libavcodec IMPORTED_TARGET)
	pkg_search_module(LIBAVUTIL REQUIRED libavutil IMPORTED_TARGET)
	pkg_search_module(LIBAVFORMAT REQUIRED libavformat IMPORTED_TARGET)
	target_link_libraries(placebo-libav-impl PkgConfig::LIBPLACEBO PkgConfig::LIBAVCODEC PkgConfig::LIBAVUTIL PkgConfig::LIBAVFORMAT)
else ()
target_link_libraries(placebo-libav-impl PkgConfig::LIBPLACEBO FFMPEG::avcodec FFMPEG::avutil FFMPEG::avformat)
endif ()
target_link_libraries(chiaki PRIVATE PkgConfig::LIBPLACEBO placebo-libav-impl)

target_link_libraries(chiaki PRIVATE chiaki-lib)
if(CHIAKI_ENABLE_CLI)
	add_definitions(-DCHIAKI_ENABLE_CLI)
        target_link_libraries(chiaki PRIVATE chiaki-cli-lib)
endif()

target_link_libraries(chiaki PRIVATE Qt::Core Qt::Gui Qt::GuiPrivate Qt::Concurrent Qt::Svg Qt::Qml Qt::Quick)

if (${Qt6DBus_FOUND} AND NOT WIN32)
        target_link_libraries(chiaki PRIVATE Qt::DBus)
	target_compile_definitions(chiaki PRIVATE CHIAKI_HAVE_DBUS)
endif()
if (ANDROID)
        pkg_search_module(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        target_link_libraries(chiaki PRIVATE PkgConfig::SDL2)

else ()
target_link_libraries(chiaki PRIVATE SDL2::SDL2)
endif ()
if(CHIAKI_ENABLE_SPEEX)
        target_link_libraries(chiaki PRIVATE PkgConfig::SpeexDSP)
	target_compile_definitions(chiaki PRIVATE CHIAKI_GUI_ENABLE_SPEEX)
endif()
if(CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER)
	target_compile_definitions(chiaki PRIVATE CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER)
endif()
if(CHIAKI_ENABLE_SETSU)
	target_link_libraries(chiaki setsu)
	target_compile_definitions(chiaki PRIVATE CHIAKI_GUI_ENABLE_SETSU)
endif()
if(CHIAKI_ENABLE_STEAMDECK_NATIVE)
        target_link_libraries(chiaki PRIVATE sdeck)
	target_compile_definitions(chiaki PRIVATE QT_STATICPLUGIN CHIAKI_GUI_ENABLE_STEAMDECK_NATIVE)
endif()
if(CHIAKI_ENABLE_STEAM_SHORTCUT)
        target_link_libraries(chiaki PRIVATE cpp-steam-tools)
	target_compile_definitions(chiaki PRIVATE CHIAKI_GUI_ENABLE_STEAM_SHORTCUT)
endif()
if(NOT CHIAKI_ENABLE_FFMPEG_DECODER)
	message(FATAL_ERROR "Chiaki GUI requires CHIAKI_ENABLE_FFMPEG_DECODER=ON")
endif()
if(APPLE)
	target_link_libraries(chiaki "-framework AVFoundation")
endif()

if (ANDROID)
    #qt_finalize_executable(chiaki)

endif ()

set_target_properties(chiaki PROPERTIES
                ${BUNDLE_ID_OPTION}
                MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME Chiaki4deck
		MACOSX_BUNDLE_BUNDLE_VERSION ${CHIAKI_VERSION}
		MACOSX_BUNDLE_COPYRIGHT "thestr4ng3r (AGPLv3)"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.chiaki4deck.chiaki4deck"
		MACOSX_BUNDLE_ICON_FILE chiaki4deck.icns
		RESOURCE "${RESOURCE_FILES}")

            if (ANDROID)
                include(GNUInstallDirs)

                install(TARGETS chiaki
                BUNDLE DESTINATION .
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                )
            else()
install(TARGETS chiaki
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin)
install(FILES chiaki4deck.desktop DESTINATION share/applications)
install(FILES chiaki4deck.png DESTINATION share/icons/hicolor/512x512/apps)
install(FILES io.github.streetpea.Chiaki4deck.appdata.xml DESTINATION share/metainfo)
endif()
