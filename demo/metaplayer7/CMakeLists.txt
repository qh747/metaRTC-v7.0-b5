cmake_minimum_required(VERSION 3.16)
project(metaplayer7 VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HOME_BASE_REL "../../")
get_filename_component(HOME_BASE ${HOME_BASE_REL} REALPATH)

if(APPLE)
    set(MACOS 1)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 OPTIONAL_COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Gui OpenGLWidgets)

qt_add_executable(metaplayer7 WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    video/yangrecordthread.cpp video/yangrecordthread.h
    yangplayer/YangPlayFactory.cpp
    yangplayer/YangPlayWidget.cpp yangplayer/YangPlayWidget.h
    yangplayer/YangPlayerBase.cpp
    yangplayer/YangPlayerDecoder.cpp
    yangplayer/YangPlayerHandleImpl.cpp yangplayer/YangPlayerHandleImpl.h
    yangplayer/YangPlayerPlay.cpp
    yangplayer/YangRtcReceive.cpp yangplayer/YangRtcReceive.h
    yangplayer/YangYuvPlayWidget.cpp yangplayer/YangYuvPlayWidget.h
)

target_include_directories(metaplayer7 PRIVATE
    ${HOME_BASE}/libmetartc7/src
    ${HOME_BASE}/thirdparty/include
    ${HOME_BASE}/demo/yangh264decoder
)

target_compile_definitions(metaplayer7 PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(metaplayer7 PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGLWidgets
)

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(metaplayer7 PRIVATE
        Qt::Widgets
    )
endif()

if(MACOS)
    target_include_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/include
    )
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/thirdparty/lib/mac
    )
    set_target_properties(metaplayer7 PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
    target_link_libraries(metaplayer7 PRIVATE
        "-framework AVFoundation"
        "-framework AudioUnit"
        "-framework CoreAudio"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework OpenGL"
        "-framework VideoToolbox"
        crypto
        dl
        metartc7
        yangwhip7
        metartccore7
        yangh264decoder
        yuv
        speexdsp
        opus
        usrsctp
        pthread
        srtp2
        ssl
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug AND MACOS)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_debug
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_debug"
    )
endif()

if(MACOS AND NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_release
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_release"
    )
endif()

if(UNIX AND NOT MACOS)
    target_include_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/include
    )
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/thirdparty/lib
    )
    target_link_libraries(metaplayer7 PRIVATE
        metartc7
        yangwhip7
        metartccore7
        yangh264decoder
        asound
        dl
        opus
        pthread
        speexdsp
        srtp2
        usrsctp
        yuv
        crypto
        ssl
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug AND UNIX AND NOT MACOS)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_debug
        ${HOME_BASE}/demo/yangh264decoder/build
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_debug"
    )
endif()

if(UNIX AND NOT CMAKE_BUILD_TYPE STREQUAL Debug AND NOT MACOS)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_release
        ${HOME_BASE}/demo/yangh264decoder/build
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_release"
    )
endif()

if(WIN32)
    target_include_directories(metaplayer7 PRIVATE
        ${HOME_BASE}\include
    )
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/thirdparty/lib/win
    )
    target_link_libraries(metaplayer7 PRIVATE
        Crypt32
        Strmiids
        avcodec
        avutil
        crypto
        ksuser
        metartc7
        metartccore7
        ole32
        openh264
        opus
        speexdsp
        srtp2
        ssl
        usrsctp
        winmm
        ws2_32
        yangwhip7
        yangh264decoder
        yuv
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug AND WIN32)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_win_debug
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_win_debug"
    )
endif()

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_directories(metaplayer7 PRIVATE
        ${HOME_BASE}/bin/lib_win_release
    )
    set_target_properties(metaplayer7 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HOME_BASE}/bin/app_win_release"
    )
endif()

if(MSVC AND WIN32)
    target_include_directories(metaplayer7 PRIVATE
        ${HOME_BASE}\thirdparty\include\win\include
    )
    target_compile_definitions(metaplayer7 PRIVATE
        HAVE_STRUCT_TIMESPEC
        WIN32_LEAN_AND_MEAN
    )
    target_compile_options(metaplayer7
        /execution-charset:utf-8
        /source-charset:utf-8
    )
endif()

install(TARGETS metaplayer7
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
