cmake_minimum_required(VERSION 3.16)

project(QVoiceBridge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets TextToSpeech Multimedia)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        piper/piper.cpp
        piper/piper.hpp
        piper/json.hpp
        piper/phoneme_ids.cpp
        piper/phoneme_ids.hpp
        piper/phonemize.cpp
        piper/phonemize.hpp
        piper/shared.cpp
        piper/shared.hpp
        piper/tashkeel.cpp
        piper/tashkeel.hpp
        piper/uni_algo.h
        piper/utf8.h

)

include_directories (
/usr/include/onnxruntime/
)

# set(ESPEAK_NG_DIR "${CMAKE_CURRENT_BINARY_DIR}/ei")




if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QVoiceBridge
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET test_tts APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(QVoiceBridge SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(QVoiceBridge
            ${PROJECT_SOURCES}
        )
    endif()
endif()

message ("AAAAAAAAAAAAAAAAAAAAa "${ESPEAK_NG_DIR}/include)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.test_tts)
endif()

set_target_properties(QVoiceBridge PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QVoiceBridge
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(QVoiceBridge)
endif()

if(NOT DEFINED ESPEAK_NG_DIR)
    # set(ESPEAK_NG_DIR "${CMAKE_CURRENT_BINARY_DIR}/ei")
    set(ESPEAK_NG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/piper/espeak-ng/")

    include(ExternalProject)
    ExternalProject_Add(
        espeak_ng_external
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/e"
        URL "${ESPEAK_NG_DIR}"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${ESPEAK_NG_DIR}
        CMAKE_ARGS -DUSE_ASYNC:BOOL=OFF
        CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=ON
        CMAKE_ARGS -DUSE_MBROLA:BOOL=OFF
        CMAKE_ARGS -DUSE_LIBSONIC:BOOL=OFF
        CMAKE_ARGS -DUSE_LIBPCAUDIO:BOOL=OFF
        CMAKE_ARGS -DUSE_KLATT:BOOL=OFF
        CMAKE_ARGS -DUSE_SPEECHPLAYER:BOOL=OFF
        CMAKE_ARGS -DEXTRA_cmn:BOOL=ON
        CMAKE_ARGS -DEXTRA_ru:BOOL=ON
        CMAKE_ARGS -DCMAKE_C_FLAGS="-D_FILE_OFFSET_BITS=64"
    )
    add_dependencies(QVoiceBridge espeak_ng_external)
endif()


target_include_directories(
    QVoiceBridge PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    ${ESPEAK_NG_DIR}/include
)

target_link_directories(
    QVoiceBridge PUBLIC
    ${ESPEAK_NG_DIR}/lib
)


target_link_libraries(QVoiceBridge PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt6::TextToSpeech
    Qt${QT_VERSION_MAJOR}::Multimedia
    espeak-ng
    onnxruntime
    fmt
)
