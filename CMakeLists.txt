cmake_minimum_required(VERSION 3.9)

project(mapex)
enable_testing()
set(CMAKE_AUTOMOC ON)

add_library(mapex.impl STATIC
  mapex/executors.hpp
  mapex/executors.cpp
  mapex/geo_point.hpp
  mapex/qnetwork_category.cpp
  mapex/qnetwork_category.hpp
  mapex/network_thread.hpp
  mapex/network_thread.cpp
  mapex/tile_loader.hpp
  mapex/tile_loader.cpp
  mapex/tile_widget.hpp
  mapex/tile_widget.cpp
)

target_compile_features(mapex.impl PUBLIC cxx_std_17)
target_include_directories(mapex.impl PUBLIC ${mapex_SOURCE_DIR})

find_package(portable_concurrency REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets Test)
target_link_libraries(mapex.impl
PUBLIC
  portable_concurrency::portable_concurrency
  Qt5::Core
  Qt5::Widgets
  Qt5::Network
)

add_executable(mapex mapex/main.cpp)
target_link_libraries(mapex PRIVATE mapex.impl)

add_executable(qnetwork_category.test mapex/qnetwork_category.test.cpp)
target_link_libraries(qnetwork_category.test PRIVATE mapex.impl Qt5::Test)
add_test(
  NAME qnetwork_category
  COMMAND qnetwork_category.test -o ${CMAKE_CURRENT_BINARY_DIR}/qnetwork_category.test.xml,xunitxml
)
