cmake_minimum_required(VERSION 3.9)

project(mapex)
add_executable(mapex main.cpp)
target_compile_features(mapex PRIVATE cxx_std_17)

find_package(portable_concurrency REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets)
target_link_libraries(mapex
PRIVATE
  portable_concurrency::portable_concurrency
  Qt5::Core
  Qt5::Widgets
  Qt5::Network
)
