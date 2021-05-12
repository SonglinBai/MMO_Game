project(test)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.15 -Wall -framework OpenGL -framework GLUT -framework Carbon")
endif()


add_executable(${PROJECT_NAME} breakout.cpp)