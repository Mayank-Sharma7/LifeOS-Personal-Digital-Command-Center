cmake_minimum_required(VERSION 3.21)
project(LifeOS VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Sql)

qt_add_executable(LifeOS
    src/main.cpp
)

target_link_libraries(LifeOS PRIVATE Qt6::Widgets Qt6::Sql)

target_compile_definitions(LifeOS PRIVATE LIFEOS_VERSION="3.0.1")

if(WIN32)
    set_target_properties(LifeOS PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "LifeOS"
    )
endif()
