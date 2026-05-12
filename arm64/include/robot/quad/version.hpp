#ifndef ROBOT_QUAD_VERSION_HPP
#define ROBOT_QUAD_VERSION_HPP

#define ROBOT_QUAD_VERSION_STRING "0.1.2"
#define ROBOT_QUAD_VERSION_MAJOR 0
#define ROBOT_QUAD_VERSION_MINOR 1
#define ROBOT_QUAD_VERSION_PATCH 2

namespace robot {
namespace quad {

inline constexpr const char* sdk_version_string() noexcept {
  return ROBOT_QUAD_VERSION_STRING;
}

inline constexpr int sdk_version_major() noexcept {
  return ROBOT_QUAD_VERSION_MAJOR;
}

inline constexpr int sdk_version_minor() noexcept {
  return ROBOT_QUAD_VERSION_MINOR;
}

inline constexpr int sdk_version_patch() noexcept {
  return ROBOT_QUAD_VERSION_PATCH;
}

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_VERSION_HPP
