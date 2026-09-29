cmake_minimum_required(VERSION 3.16)
project(mota LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    enable_language(RC)
endif()

# Enable Qt automoc/autorcc
set(CMAKE_AUTOMOC ON)
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
    UI/MenuWindow.cpp
    UI/SlidingPuzzlePage.cpp
    UI/MainWindow.cpp
    resources.qrc
)

add_executable(mota ${SRC})

if(WIN32)
    target_sources(mota PRIVATE app_icon.rc)
endif()

target_include_directories(mota PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mota PRIVATE Qt6::Widgets)

add_executable(mota_classic_tests
    tests/test_classic_items.cpp
    Game/Game.cpp
    Entities/Player.cpp
    Entities/Items.cpp
    Entities/Monster.cpp
    Entities/NPC.cpp
)
target_include_directories(mota_classic_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mota_classic_tests PRIVATE Qt6::Widgets)

add_executable(mota_motion_tests
    tests/test_player_motion.cpp
)
target_include_directories(mota_motion_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mota_battle_feedback_tests
    tests/test_battle_feedback.cpp
    Entities/Monster.cpp
)
target_include_directories(mota_battle_feedback_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mota_mainwindow_input_tests
    tests/test_mainwindow_input.cpp
    Game/Game.cpp
    Entities/Player.cpp
    Entities/Items.cpp
    Entities/Monster.cpp
    Entities/NPC.cpp
    UI/MapWidget.cpp
    UI/MainWindow.cpp
    resources.qrc
)
target_include_directories(mota_mainwindow_input_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mota_mainwindow_input_tests PRIVATE Qt6::Widgets)

add_executable(mota_story_script_tests tests/test_story_script.cpp)
target_include_directories(mota_story_script_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mota_puzzle_tests tests/test_sliding_puzzle.cpp UI/SlidingPuzzlePage.cpp resources.qrc)
target_include_directories(mota_puzzle_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mota_puzzle_tests PRIVATE Qt6::Widgets)
