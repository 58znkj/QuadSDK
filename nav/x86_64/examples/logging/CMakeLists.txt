if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(quad_example_logging LANGUAGES CXX)

  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  find_package(robot CONFIG REQUIRED)
endif()

add_executable(quad_example_logging
  main.cpp
)

target_include_directories(quad_example_logging PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(quad_example_logging PRIVATE
  robot::sdk::quad
)

if(UNIX AND NOT APPLE)
  set_target_properties(quad_example_logging PROPERTIES
    BUILD_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH "$ORIGIN/../lib")
endif()

if(ROBOT_INSTALL)
  install(TARGETS quad_example_logging
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT robot)
endif()
