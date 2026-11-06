#ifndef ROBOT_QUAD_NAV_RESOURCE_MODULE_HPP
#define ROBOT_QUAD_NAV_RESOURCE_MODULE_HPP

#include <robot/quad/common/types.hpp>
#include <robot/quad/nav/types.hpp>
#include <robot/quad/export.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace robot {
namespace quad {
namespace nav {

/**
 * @file resource_module.hpp
 * @brief Quad 地图资源模块。
 */

/**
 * @brief Quad 地图资源模块。
 *
 * 该模块负责地图资源缓存刷新、资源缓存读取、地图导出，以及场景/路径管理。
 */
class ROBOT_QUAD_API ResourceModule {
public:
  /**
   * @brief 销毁资源模块。
   */
  ~ResourceModule();

  ResourceModule(const ResourceModule&) = delete;
  ResourceModule& operator=(const ResourceModule&) = delete;

  /**
   * @brief 从机器人拉取场景与轨迹资源，并刷新 SDK 本地缓存。
   *
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   *
   * @note 命令回调后仍建议检查 `getMapResources()` 返回值中的
   *       `scenes_valid` 与 `trajectories_valid` 字段，确认缓存可用。
   */
  ::robot::quad::AsyncCommandId refreshMapResources(
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 获取当前地图资源缓存快照。
   *
   * @return 资源缓存快照。
   */
  ::robot::quad::MapResourceSnapshot getMapResources() const;

  /**
   * @brief 重命名指定路径。
   *
   * @param scene_name 路径所属场景名称。
   * @param old_name 原路径名称。
   * @param new_name 新路径名称。
   * @param callback 异步命令回调；回调结果表示命令受理状态。
   * @return 异步命令 ID。非零表示请求已受理，`0` 表示请求未入队。
   */
  ::robot::quad::AsyncCommandId renamePath(
      const std::string& scene_name,
      const std::string& old_name,
      const std::string& new_name,
      ::robot::quad::AsyncCommandCallback callback = {});

  /**
   * @brief 导出指定子场景地图到本地目录。
   *
   * @param scene_name 场景名称。
   * @param sub_scene_id 子场景 ID。
   * @param save_dir 本地保存目录。
   * @param callback 导出结果回调，返回统一错误码。
   * @param options 导出超时参数。
   */
  void exportMapToLocal(const std::string& scene_name,
                        uint32_t sub_scene_id,
                        const std::string& save_dir,
                        ::robot::quad::MapTransferCallback callback,
                        ::robot::quad::MapTransferOptions options = {});

  /**
   * @brief 订阅地图资源事件。
   *
   * @param callback 资源事件回调。
   * @param options 订阅选项：`replay_latest` 表示注册后先回放当前缓存，
   *                `only_changed` 表示仅在状态变化时回调，
   *                `fresh_only` 表示忽略非新鲜缓存。
   * @return 订阅句柄；析构或调用 `unsubscribe()` 后停止接收回调。
   */
  ::robot::quad::Subscription registerMapResourceEvent(
      ::robot::quad::MapResourceEventCallback callback,
      ::robot::quad::SubscribeOptions options = {});

private:
  friend class Domain;
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ResourceModule(void* navigator);
};

}  // namespace nav
}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_NAV_RESOURCE_MODULE_HPP
