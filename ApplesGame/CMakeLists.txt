cmake_minimum_required(VERSION 3.16)
project(SnakeGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)

add_executable(SnakeGame
    ApplesGame/main.cpp
    ApplesGame/Game.cpp
    ApplesGame/Records.cpp
)

target_include_directories(SnakeGame PRIVATE src)
target_link_libraries(SnakeGame PRIVATE sfml-graphics sfml-audio)

# копирование assets рядом с бинарником
add_custom_command(TARGET SnakeGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:SnakeGame>/assets
)