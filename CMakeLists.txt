cmake_minimum_required(VERSION 3.16)
project(mota LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Enable Qt automoc/autouic/autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(mota
    main.cpp
    Game/Game.cpp
    Entities/Player.cpp
    UI/MainWindow.cpp
)

target_include_directories(mota PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mota PRIVATE Qt6::Widgets)
