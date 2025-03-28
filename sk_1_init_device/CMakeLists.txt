cmake_minimum_required(VERSION 3.7)
 
project(sk1_ini_dev LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14) 

find_package(UHD 4.1.0 REQUIRED)

include_directories(${PROJECT_NAME} PRIVATE ${UHD_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${UHD_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
)

