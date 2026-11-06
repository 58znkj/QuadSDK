#ifndef ROBOT_PUBLIC_QUAD_COMMON_TYPES_HPP
#define ROBOT_PUBLIC_QUAD_COMMON_TYPES_HPP

/**
 * @file common/types.hpp
 * @brief Quad SDK 通用类型定义，包括几何数据、订阅句柄与异步命令结果。
 */

#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <robot/quad/export.hpp>
#include <robot/quad/error.hpp>
#include <string>
#include <utility>

namespace robot {
namespace quad {

namespace subscription_detail {
struct SubscriptionAccess;
}  // namespace subscription_detail

/** @brief 三维点坐标。 */
struct Point3D {
    float x{0};    /**< X 坐标。 */
    float y{0};    /**< Y 坐标。 */
    float z{0};    /**< Z 坐标。 */
};

/** @brief 四元数姿态。 */
struct Quaternion {
    float x{0};    /**< x 分量。 */
    float y{0};    /**< y 分量。 */
    float z{0};    /**< z 分量。 */
    float w{1};    /**< w 分量。 */
};

/** @brief 位置与朝向组成的位姿。 */
struct Pose {
    Point3D    position;       /**< 位置。 */
    Quaternion orientation;    /**< 姿态。 */
};

/** @brief 二维平面位姿。 */
struct Pose2D {
    float x{0};             /**< X 坐标。 */
    float y{0};             /**< Y 坐标。 */
    float yaw_degree{0};    /**< 绕 Z 轴的偏航角，单位为度。 */
};

/**
 * @brief 从三维位姿中提取二维平面位姿。
 *
 * 当前 Quad 定点导航仅使用 `Pose` 中的二维平面信息，因此该函数可用于将
 * `Pose` 转换为更直观的 `x`、`y` 与 `yaw_degree` 表达。
 *
 * @param pose 三维位姿。
 * @return 提取后的二维平面位姿；偏航角范围归一化到 `[-180, 180)`。
 */
inline Pose2D pose2DFromPose(const Pose& pose) {
    constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;
    const double     x = static_cast<double>(pose.orientation.x);
    const double     y = static_cast<double>(pose.orientation.y);
    const double     z = static_cast<double>(pose.orientation.z);
    const double     w = static_cast<double>(pose.orientation.w);
    const double yaw_rad =
        std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z));
    double yaw_degree = yaw_rad * kRadToDeg;
    yaw_degree = std::fmod(yaw_degree + 180.0, 360.0);
    if (yaw_degree < 0.0) {
        yaw_degree += 360.0;
    }

    Pose2D planar_pose{};
    planar_pose.x = pose.position.x;
    planar_pose.y = pose.position.y;
    planar_pose.yaw_degree = static_cast<float>(yaw_degree - 180.0);
    return planar_pose;
}

/**
 * @brief 由二维平面位姿构造 `Pose`。
 *
 * 该函数适用于当前仅支持二维平面的定点导航接口：`z` 可单独指定，
 * 姿态只根据绕 Z 轴的偏航角构造。
 *
 * @param pose_2d 二维平面位姿。
 * @param z 目标 Z 坐标。
 * @return 构造后的三维位姿。
 */
inline Pose poseFromPose2D(const Pose2D& pose_2d, float z = 0.0f) {
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
    double rotation_deg = static_cast<double>(pose_2d.yaw_degree);
    rotation_deg = std::fmod(rotation_deg + 360.0, 360.0);
    if (rotation_deg >= 180.0) {
        rotation_deg -= 360.0;
    }
    const double half_rad = (rotation_deg * 0.5) * kDegToRad;

    Pose pose{};
    pose.position.x = pose_2d.x;
    pose.position.y = pose_2d.y;
    pose.position.z = z;
    pose.orientation.z = static_cast<float>(std::sin(half_rad));
    pose.orientation.w = static_cast<float>(std::cos(half_rad));
    return pose;
}

/**
 * @brief 由 `x`、`y` 与偏航角快速构造 `Pose`。
 *
 * @param x 目标 X 坐标。
 * @param y 目标 Y 坐标。
 * @param yaw_degree 绕 Z 轴的偏航角，单位为度。
 * @param z 目标 Z 坐标。
 * @return 构造后的三维位姿。
 */
inline Pose poseFromXYYawDegrees(float x,
                                 float y,
                                 float yaw_degree,
                                 float z = 0.0f) {
    return poseFromPose2D(Pose2D{x, y, yaw_degree}, z);
}

/**
 * @brief 订阅生命周期句柄。
 *
 * 句柄析构、移动赋值或显式调用 unsubscribe() 时，都会至多触发一次清理逻辑。
 */
class ROBOT_QUAD_API Subscription {
private:
    explicit Subscription(std::shared_ptr<void> state);
    friend struct subscription_detail::SubscriptionAccess;

public:
    Subscription() = default;

    /** @brief 析构并自动退订。 */
    ~Subscription();

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    /** @brief 通过移动构造接管另一个订阅句柄。 */
    Subscription(Subscription&& other) noexcept;

    /** @brief 通过移动赋值接管另一个订阅句柄，必要时先退订当前句柄。 */
    Subscription& operator=(Subscription&& other) noexcept;

    /** @brief 主动退订；重复调用安全。 */
    void unsubscribe();

    /** @brief 返回当前句柄是否仍绑定到有效订阅。 */
    bool valid() const;

private:
    std::shared_ptr<void> state_;
};

/** @brief 订阅行为选项。 */
struct SubscribeOptions {
    bool replay_latest{true};    /**< 订阅建立后是否立即回放最近一次缓存值。 */
    bool only_changed{false};    /**< 是否仅在内容发生变化时触发回调。 */
    bool fresh_only{false};      /**< 是否仅接收当前连接周期内的新鲜数据。 */
};

/** @brief 异步命令 ID。 */
using AsyncCommandId = uint64_t;

/** @brief 异步命令执行状态。 */
enum class AsyncCommandStatus {
    SUCCEEDED = 0,         /**< 命令成功完成。 */
    TIMEOUT = 1,           /**< 命令等待超时。 */
    PUBLISH_FAILED = 2,    /**< 命令发布失败。 */
    CANCELLED = 3,         /**< 命令被取消。 */
    SHUTDOWN = 4,          /**< SDK 运行时已关闭。 */
    FAILED = 5,            /**< 命令执行失败。 */
};

/** @brief 异步命令执行结果。 */
struct AsyncCommandResult {
    AsyncCommandId     command_id{0};                         /**< 命令 ID。 */
    AsyncCommandStatus status{AsyncCommandStatus::FAILED};    /**< 执行状态。 */
    ErrorCode          error{ErrorCode::SUCCESS};             /**< 统一错误码。 */
    std::string        command_name;                          /**< 命令名称。 */
    std::string        detail;                                /**< 附加诊断信息。 */
};

/** @brief 异步命令结果回调。 */
using AsyncCommandCallback = std::function<void(const AsyncCommandResult&)>;

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_COMMON_TYPES_HPP
