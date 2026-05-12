#ifndef ROBOT_QUAD_NAV_MAPPING_MODULE_HPP
#define ROBOT_QUAD_NAV_MAPPING_MODULE_HPP

#include <robot/quad/common/types.hpp>
#include <robot/quad/nav/types.hpp>
#include <robot/quad/export.hpp>

#include <memory>
#include <string>

namespace robot {
namespace quad {
namespace nav {

/**
 * @file mapping_module.hpp
 * @brief Quad 定位模块。
 */

/**
 * @brief Quad 定位模块。
 *
 * 出于目录稳定性与历史命名兼容，类名仍沿用 `MappingModule`。
 * 在当前 Quad facade 中，该模块对外提供定位控制、状态读取与状态订阅能力。
 */
class ROBOT_QUAD_API MappingModule {
public:
  /**
   * @brief 销毁定位模块。
   */
  ~MappingModule();

  MappingModule(const MappingModule&) = delete;
  MappingModule& operator=(const MappingModule&) = delete;

  /**
   * @brief 基于指定场景启动定位。
   *
   * @param scene_name 场景名称，建议先通过 `resource().refreshMapResources()` 刷新资源缓存。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   *
   * @note 当前 Quad 示例中，直接以该异步回调结果作为定位启动结果判断。
   *       如业务还需要读取定位缓存，可继续调用 `getLocalizationState()`。
   */
  ::robot::quad::AsyncCommandId startLocalization(
      const std::string& scene_name,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 基于指定场景和初始位姿启动定位。
   *
   * @param scene_name 场景名称，建议先通过 `resource().refreshMapResources()` 刷新资源缓存。
   * @param initial_pose 提供给定位模块的初始位姿。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId startLocalization(
      const std::string& scene_name,
      const ::robot::quad::Pose& initial_pose,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 停止当前定位任务。
   *
   * @param scene_name 当前定位所对应的场景名称。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId stopLocalization(
      const std::string& scene_name,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 获取当前定位状态缓存。
   *
   * @return 当前定位状态。
   *
   * 该接口用于读取当前定位缓存。
   */
  ::robot::quad::LocalizationState getLocalizationState() const;

  /**
   * @brief 订阅定位状态变化。
   *
   * @param callback 定位状态回调。
   * @param options 订阅选项：`replay_latest` 表示注册后先回放当前缓存，
   *                `only_changed` 表示仅在状态变化时回调，
   *                `fresh_only` 表示忽略非新鲜缓存。
   * @return 订阅句柄；析构或调用 `unsubscribe()` 后停止接收回调。
   *
   * @note 当底层 `/PNS/PnsSlamInfo` 或 `/PNS/Position` 更新时，SDK 会重新计算
   *       `LocalizationState` 并通过该接口分发。
   */
  ::robot::quad::Subscription registerLocalizationStateEvent(
      ::robot::quad::LocalizationStateEventCallback callback,
      ::robot::quad::SubscribeOptions options = {});

private:
  friend class Domain;
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit MappingModule(void* navigator);
};

}  // namespace nav
}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_NAV_MAPPING_MODULE_HPP
