#ifndef ROBOT_PUBLIC_QUAD_MOTION_TYPES_HPP
#define ROBOT_PUBLIC_QUAD_MOTION_TYPES_HPP

/**
 * @file motion/types.hpp
 * @brief Quad SDK 运动域类型定义。
 */

#include <cstdint>

namespace robot {
namespace quad {

/** @brief 机器人步态类型。 */
enum class GaitType {
    WALK = 0,    /**< 行走步态。 */
    RUN = 1,     /**< 奔跑步态。 */
};

/** @brief 机器人运动模式。 */
enum class MotionMode : uint32_t {
    Manual = 0,        /**< 手动控制模式。 */
    Assisted = 1,      /**< 辅助控制模式。 */
    Navigation = 2,    /**< 导航模式。 */
};

/** @brief 当前运动状态。 */
struct MotionState {
    GaitType gait{GaitType::WALK};    /**< 当前步态。 */
};

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_MOTION_TYPES_HPP
