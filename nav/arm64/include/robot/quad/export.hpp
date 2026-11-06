#ifndef ROBOT_QUAD_EXPORT_HPP
#define ROBOT_QUAD_EXPORT_HPP

#ifdef ROBOT_QUAD_BUILDING
#  define ROBOT_QUAD_API __attribute__((visibility("default")))
#else
#  define ROBOT_QUAD_API
#endif

#endif  // ROBOT_QUAD_EXPORT_HPP
