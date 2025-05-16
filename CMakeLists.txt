cmake_minimum_required(VERSION 3.10)
project(Flurry)
find_package(OpenCV REQUIRED)
add_executable(Flurry flurry.cpp)
target_link_libraries(Flurry ${OpenCV_LIBS} :libpaho-mqtt3c.a)

