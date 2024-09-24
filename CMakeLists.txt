cmake_minimum_required(VERSION 3.14)

project(glassd LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
message(STATUS "Found Qt ${QT_VERSION_MAJOR}")
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus Qml Gui Quick)

add_executable(glassd
    Glassd.h
    Glassd_dbus.h
  main.cpp
  Glassd.cpp
  qml/viz_default.qml viz_wrangler.cpp
  Gesture_mapper.h Gesture_mapper.cpp
  Gesture_handlers.h

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
install(FILES "63-glassd-uinput.rules"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/udev/rules.d
)
install(FILES "glassd-kwin/contents/code/main.js"
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/glassd/kwin-integration)
