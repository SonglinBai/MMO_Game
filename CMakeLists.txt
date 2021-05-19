cmake_minimum_required(VERSION 3.19)
project(MMO_Game)

set(CMAKE_CXX_STANDARD 17)

set(INCLUDES
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/asio/include
        ${PROJECT_SOURCE_DIR}/external/network/include
        ${PROJECT_SOURCE_DIR}/external/pixelGameEngine/extension
        ${PROJECT_SOURCE_DIR}/external/stb_image/include
        ${PROJECT_SOURCE_DIR}/external/magic_enum/include
        )

if (WIN32)
    include_directories(${INCLUDES} ${PROJECT_SOURCE_DIR}/external/pixelGameEngine/include)
endif ()

if (APPLE)
    include_directories(${INCLUDES} ${PROJECT_SOURCE_DIR}/external/pixelGameEngine/include_mac)
endif ()


add_subdirectory(MMO_Client)
add_subdirectory(MMO_Server)

