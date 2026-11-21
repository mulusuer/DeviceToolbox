cmake_minimum_required(VERSION 3.20)
project(DeviceToolbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# CMAKE_PREFIX_PATH：Windows 本机 Qt 路径；Linux 由命令行传入或系统自动查找。
if (WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/Qt/6.8.3/mingw_64")
endif ()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network SerialPort Sql Charts)

add_executable(DeviceToolbox
        src/main.cpp
        src/mainwindow.cpp src/mainwindow.h
        src/device.cpp src/device.h
        src/devices.cpp src/devices.h
        src/devicepanel.cpp src/devicepanel.h
        src/frameparser.cpp src/frameparser.h
        src/storage.cpp src/storage.h
        src/modbusmaster.cpp src/modbusmaster.h
)

target_include_directories(DeviceToolbox PRIVATE src)
target_link_libraries(DeviceToolbox PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::SerialPort Qt6::Sql Qt6::Charts)

# 每次构建用 windeployqt 自动补齐运行库；Linux 下整段跳过。
if (WIN32)
    add_custom_command(TARGET DeviceToolbox POST_BUILD
            COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe"
                    --no-translations "$<TARGET_FILE:DeviceToolbox>"
            COMMENT "Deploying Qt runtime libraries")
endif ()
