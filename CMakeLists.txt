cmake_minimum_required(VERSION 3.14)

project(glassd LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

find_package(Qt6 REQUIRED COMPONENTS DBus Qml Gui Quick)

add_executable(glassd
  main.cpp
  Glassd.h Glassd.cpp
  Glassd_dbus.h
  qml/viz_default.qml
  viz_wrangler.h viz_wrangler.cpp
  #glassd_copy.cpp
)
target_link_libraries(glassd
    Qt${QT_VERSION_MAJOR}::Core
    evdev
    omniglass_linux
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
target_include_directories(glassd PRIVATE
    /usr/include/libevdev-1.0
)

qt_add_resources(glassd "visualizer_qml"
    PREFIX "/"
    FILES
        "qml/viz_default.qml"
)

include(GNUInstallDirs)
install(TARGETS glassd
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
