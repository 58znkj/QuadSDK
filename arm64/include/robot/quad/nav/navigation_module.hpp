#ifndef ROBOT_QUAD_NAV_NAVIGATION_MODULE_HPP
#define ROBOT_QUAD_NAV_NAVIGATION_MODULE_HPP

#include <robot/quad/common/types.hpp>
#include <robot/quad/motion/types.hpp>
#include <robot/quad/nav/types.hpp>
#include <robot/quad/export.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace robot {
namespace quad {
namespace nav {

/**
 * @file navigation_module.hpp
 * @brief Quad 导航控制模块。
 */

/**
 * @brief Quad 导航控制模块。
 *
 * 该模块负责运动模式切换、定点/循迹导航、暂停/恢复与导航状态订阅。
 * 公开控制接口均为异步接口：函数返回时仅表示请求已提交，
 * 实际受理结果通过 `AsyncCommandCallback` 返回，导航执行状态通过 `NavigationState` 观察。
 */
class ROBOT_QUAD_API NavigationModule {
public:
  /**
   * @brief 销毁导航模块。
   */
  ~NavigationModule();

  NavigationModule(const NavigationModule&) = delete;
  NavigationModule& operator=(const NavigationModule&) = delete;

  /**
   * @brief 切换机器人运动模式。
   *
   * @param mode 目标运动模式。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId setMotionMode(
      ::robot::quad::MotionMode mode,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 获取当前运动模式缓存。
   *
   * @return 当前缓存的运动模式。
   */
  ::robot::quad::MotionMode getMotionMode() const;

  /**
   * @brief 发起定点导航。
   *
   * @param scene_name 目标场景名称。
   * @param target 目标位姿（位置与四元数朝向）。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   *
   * @note 当前定点导航仅支持二维平面，实际只使用 `target.position.x`、
   *       `target.position.y` 与绕 Z 轴偏航角；可配合 `pose2DFromPose()`、
   *       `poseFromPose2D()` 或 `poseFromXYYawDegrees()` 使用。
   */
  ::robot::quad::AsyncCommandId startNavigation(
      const std::string& scene_name,
      const ::robot::quad::Pose& target,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 发起循迹导航。
   *
   * @param scene_name 路径所属场景名称。
   * @param path_id 轨迹路径 ID，可从 `MapResourceSnapshot::trajectories` 中获取。
   * @param forward `true` 表示正向循迹，`false` 表示反向循迹。
   * @param obstacle_timeout_s 障碍等待超时时间，单位为秒。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId startNavigation(
      const std::string& scene_name,
      int32_t path_id,
      bool forward = true,
      uint32_t obstacle_timeout_s = 30,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 停止当前导航任务。
   *
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId stopNavigation(
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 设置导航暂停或恢复。
   *
   * @param paused `true` 表示暂停，`false` 表示恢复。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId setNavigationPaused(
      bool paused,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 获取当前导航暂停状态缓存。
   *
   * @return 当前暂停状态。
   */
  ::robot::quad::PauseState getPauseState() const;

  /**
   * @brief 订阅导航暂停状态。
   *
   * @param callback 暂停状态回调。
   * @param options 订阅选项：`replay_latest` 表示注册后先回放当前缓存，
   *                `only_changed` 表示仅在状态变化时回调，
   *                `fresh_only` 表示忽略非新鲜缓存。
   * @return 订阅句柄；析构或调用 `unsubscribe()` 后停止接收回调。
   */
  ::robot::quad::Subscription registerPauseStateEvent(
      ::robot::quad::PauseStateEventCallback callback,
      ::robot::quad::SubscribeOptions options = {});

  /**
   * @brief 获取当前导航状态缓存。
   *
   * @return 当前导航状态快照。
   *
   * @note `error` 为 SDK 映射后的统一错误码，`error_message` 保留原始描述信息便于排障。
   */
  ::robot::quad::NavigationState getNavigationState() const;

  /**
   * @brief 获取最近一次导航终态结果缓存。
   *
   * @return 最近一次导航终态结果。
   */
  ::robot::quad::NavigationResult getNavigationResult() const;

  /**
   * @brief 订阅导航状态变化。
   *
   * @param callback 导航状态回调。
   * @param options 订阅选项：`replay_latest` 表示注册后先回放当前缓存，
   *                `only_changed` 表示仅在状态变化时回调，
   *                `fresh_only` 表示忽略非新鲜缓存。
   * @return 订阅句柄；析构或调用 `unsubscribe()` 后停止接收回调。
   */
  ::robot::quad::Subscription registerNavigationStateEvent(
      ::robot::quad::NavigationStateEventCallback callback,
      ::robot::quad::SubscribeOptions options = {});

  /**
   * @brief 订阅导航终态结果事件。
   *
   * @param callback 导航结果回调。
   * @param options 订阅选项：`replay_latest` 表示注册后先回放当前缓存，
   *                `only_changed` 表示仅在状态变化时回调，
   *                `fresh_only` 表示忽略非新鲜缓存。
   * @return 订阅句柄；析构或调用 `unsubscribe()` 后停止接收回调。
   */
  ::robot::quad::Subscription registerNavigationResultEvent(
      ::robot::quad::NavigationResultEventCallback callback,
      ::robot::quad::SubscribeOptions options = {});

private:
  friend class Domain;
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit NavigationModule(void* navigator);
};

}  // namespace nav
}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_NAV_NAVIGATION_MODULE_HPP
