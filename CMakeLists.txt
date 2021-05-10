cmake_minimum_required(VERSION 3.19)
project(MMO_Game)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/asio/include
        ${PROJECT_SOURCE_DIR}/external/network/include
        ${PROJECT_SOURCE_DIR}/external/pixelGameEngine/include
        ${PROJECT_SOURCE_DIR}/external/pixelGameEngine/extension
        )

add_subdirectory(MMO_Client)
add_subdirectory(MMO_Server)

