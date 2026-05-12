#ifndef ROBOT_QUAD_NAV_DOMAIN_HPP
#define ROBOT_QUAD_NAV_DOMAIN_HPP

#include <robot/quad/export.hpp>
#include <robot/quad/nav/mapping_module.hpp>
#include <robot/quad/nav/navigation_module.hpp>
#include <robot/quad/nav/resource_module.hpp>

#include <memory>

namespace robot {
namespace quad {

class Robot;

namespace nav {

/**
 * @file domain.hpp
 * @brief Quad 导航业务域入口。
 */

/**
 * @brief Quad 导航业务域。
 *
 * `Domain` 把 Quad 用户库对外暴露的能力收敛为三类：
 * - `navigation()`：运动模式与导航控制。
 * - `mapping()`：定位控制与定位状态读取。
 * - `resource()`：地图资源缓存刷新、查询、导出与资源管理。
 */
class ROBOT_QUAD_API Domain {
public:
  /**
   * @brief 销毁业务域对象。
   */
  ~Domain();

  Domain(const Domain&) = delete;
  Domain& operator=(const Domain&) = delete;

  /**
   * @brief 访问导航控制模块。
   *
   * @return 导航控制模块引用。
   */
  NavigationModule& navigation();

  /**
   * @brief 访问定位模块。
   *
   * @return 定位模块引用。
   */
  MappingModule& mapping();

  /**
   * @brief 访问地图资源模块。
   *
   * @return 地图资源模块引用。
   */
  ResourceModule& resource();

private:
  friend class ::robot::quad::Robot;

  explicit Domain(void* navigator);

  std::unique_ptr<NavigationModule> navigation_;
  std::unique_ptr<MappingModule> mapping_;
  std::unique_ptr<ResourceModule> resource_;
};

}  // namespace nav
}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_NAV_DOMAIN_HPP
