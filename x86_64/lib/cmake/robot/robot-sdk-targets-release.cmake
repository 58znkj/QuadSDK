#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "robot::robot_sdk_quad" for configuration "Release"
set_property(TARGET robot::robot_sdk_quad APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(robot::robot_sdk_quad PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libquad.so.0.1.3"
  IMPORTED_SONAME_RELEASE "libquad.so.0"
  )

list(APPEND _cmake_import_check_targets robot::robot_sdk_quad )
list(APPEND _cmake_import_check_files_for_robot::robot_sdk_quad "${_IMPORT_PREFIX}/lib/libquad.so.0.1.3" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
