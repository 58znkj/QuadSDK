# Robot SDK CMake configuration
set(robot_VERSION 0.1.2)
set(robot_VERSION_MAJOR 0)
set(robot_VERSION_MINOR 1)
set(robot_VERSION_PATCH 2)

# Find dependency
include(CMakeFindDependencyMacro)
find_dependency(Threads REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/robot-sdk-targets.cmake")

if(TARGET robot::robot_sdk_quad AND NOT TARGET robot::sdk::quad)
  add_library(robot::sdk::quad INTERFACE IMPORTED)
  set_target_properties(robot::sdk::quad PROPERTIES
    INTERFACE_LINK_LIBRARIES robot::robot_sdk_quad)
endif()
