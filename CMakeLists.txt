cmake_minimum_required(VERSION 3.16)
project(cutefish-store VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(APPSTREAM REQUIRED appstream)

include_directories(/usr/include/cutefish /usr/include/cutefish-toolkit /usr/include/cutefish-toolkit-base ${APPSTREAM_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} src/main.cpp src/catalog.h)
target_link_libraries(${PROJECT_NAME} PRIVATE cutefish-toolkit cutefish-toolkit-base cutefish-packagekit-client ${APPSTREAM_LIBRARIES})
