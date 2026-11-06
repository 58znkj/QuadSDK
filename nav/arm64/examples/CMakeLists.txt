if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(robot_quad_examples LANGUAGES CXX)

  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  find_package(robot CONFIG REQUIRED)
endif()

add_subdirectory(logging)
add_subdirectory(multi_point_nav_queue)
add_subdirectory(pose_nav_workflow)
