cmake_minimum_required(VERSION 3.16)
project(mota LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Enable Qt automoc/autouic/autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Sources
set(SRC
    main.cpp
    Game/Game.cpp
    Entities/Player.cpp
    Entities/Items.cpp
    Entities/Monster.cpp
    Entities/NPC.cpp
    UI/MapWidget.cpp
    UI/MapEditor.cpp
    UI/MenuWindow.cpp
    UI/MainWindow.cpp
    resources.qrc
)

add_executable(mota ${SRC})

# Ensure build directory is in include path so generated ui_*.h are found
target_include_directories(mota PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(mota PRIVATE Qt6::Widgets)
