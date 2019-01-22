cmake_minimum_required(VERSION 3.9)

project(mapex)
enable_testing()
set(CMAKE_AUTOMOC ON)

add_library(mapex.impl STATIC
  mapex/deltapack.hpp
  mapex/executors.hpp
  mapex/executors.cpp
  mapex/geo_point.hpp
  mapex/qnetwork_category.cpp
  mapex/qnetwork_category.hpp
  mapex/morton_code.hpp
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

set(TESTS_SRC
  mapex/qnetwork_category.test.cpp
  mapex/deltapack.test.cpp
)

foreach(src ${TESTS_SRC})
  get_filename_component(tst ${src} NAME_WE)
  set(tgt ${tst}.test)
  add_executable(${tgt} ${src})
  target_link_libraries(${tgt} PRIVATE mapex.impl Qt5::Test)
  add_test(
    NAME ${tst}
    COMMAND ${tgt} -o ${CMAKE_CURRENT_BINARY_DIR}/${tst}.test.xml,xunitxml
  )
endforeach()
