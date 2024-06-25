cmake_minimum_required(VERSION 3.14)

project(glassd LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(glassd
  main.cpp
  glassd.h glassd.cpp
  #glassd_copy.cpp
)
target_link_libraries(glassd
    Qt${QT_VERSION_MAJOR}::Core
    evdev
    omniglass_linux
)
target_include_directories(glassd PRIVATE
    /usr/include/libevdev-1.0
)
include(GNUInstallDirs)
install(TARGETS glassd
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
