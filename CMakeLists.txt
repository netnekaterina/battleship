cmake_minimum_required(VERSION 3.10)
project(BattleshipWithMines)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(battleship 
    src/main.cpp
    src/GameBoard.cpp
    src/BattleshipAlgorithm.cpp
)

target_include_directories(battleship PRIVATE include) 