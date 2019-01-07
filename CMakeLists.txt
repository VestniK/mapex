cmake_minimum_required(VERSION 3.9)

project(mapex)
set(CMAKE_AUTOMOC ON)

add_executable(mapex
  mapex/geo_point.hpp
  mapex/main.cpp
  mapex/promised_reply.cpp
  mapex/promised_reply.hpp
  mapex/tile_loader.hpp
  mapex/tile_loader.cpp
  mapex/tile_widget.hpp
  mapex/tile_widget.cpp
)
target_compile_features(mapex PRIVATE cxx_std_17)
target_include_directories(mapex PRIVATE ${mapex_SOURCE_DIR})

find_package(portable_concurrency REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets)
target_link_libraries(mapex
PRIVATE
  portable_concurrency::portable_concurrency
  Qt5::Core
  Qt5::Widgets
  Qt5::Network
)
