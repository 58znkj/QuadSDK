#ifndef ROBOT_QUAD_ROBOT_HPP
#define ROBOT_QUAD_ROBOT_HPP

#include <robot/quad/robot/types.hpp>
#include <robot/quad/export.hpp>
#include <robot/quad/nav/domain.hpp>

#include <chrono>
#include <memory>
#include <string>

namespace robot {
namespace quad {

/**
 * @file robot.hpp
 * @brief Quad SDK 的主入口类型。
 */

/**
 * @brief Quad 用户库主入口。
 *
 * 典型使用流程：
 * 1. 构造 `Robot` 并指定机器人 IP 等连接参数。
 * 2. 调用 connect(timeout) 建立连接。
 * 3. 通过 nav().resource() 同步并查询地图资源。
 * 4. 通过 nav().mapping() 启动定位。
 * 5. 通过 nav().navigation() 设置运动模式、发起导航并订阅导航状态。
 *
 * 日志配置采用全局静态接口，对当前进程内全部 Quad SDK 实例生效。
 */
class ROBOT_QUAD_API Robot {
public:
  /**
   * @brief 创建一个 Quad 机器人对象。
   *
   * @param options 连接参数与 SDK 标识。
   */
  explicit Robot(const ::robot::quad::RobotOptions& options = ::robot::quad::RobotOptions{});

  /**
   * @brief 销毁机器人对象并释放内部资源。
   */
  ~Robot();

  Robot(const Robot&) = delete;
  Robot& operator=(const Robot&) = delete;

  /**
   * @brief 在给定超时内同步建立连接并等待机器人首个心跳。
   *
   * 成功条件为：MQTT 传输层已连接、必需订阅已建立，并已收到当前连接周期的首个心跳。
   * 若在超时前未进入就绪状态，则本次连接尝试会被主动清理并返回失败。
   *
   * @param timeout 等待连接就绪的最长时长。
   * @return 在超时前进入就绪状态返回 `true`，否则返回 `false`。
   */
  bool connect(std::chrono::milliseconds timeout);

  /**
   * @brief 断开与机器人的连接。
   */
  void disconnect();

  /**
   * @brief 查询 SDK 当前是否处于连接状态。
   *
   * @return `true` 表示连接已建立。
   */
  bool isConnected() const;

  /**
   * @brief 查询机器人在线状态缓存。
   *
   * @return `true` 表示机器人心跳正常。
   */
  bool isRobotOnline() const;

  /**
   * @brief 获取当前机器人 IP。
   *
   * @return 机器人 IP 字符串。
   */
  std::string getRobotIp() const;

  /**
   * @brief 访问导航业务域。
   *
   * 通过该入口访问资源、定位和导航三个用户模块。
   *
   * @return Quad 导航业务域对象引用。
   */
  nav::Domain& nav();

  /**
   * @brief 同时设置控制台与文件日志等级。
   *
   * 该配置为进程级全局配置，会影响后续创建或已存在的 Quad SDK 实例。
   * 未显式配置时，控制台默认为 `LogLevel::WARN`，文件默认为 `LogLevel::DEBUG`。
   *
   * @note 在首次创建 `Robot` 之前，还可通过环境变量覆盖：`ROBOT_QUAD_LOG_LEVEL`（仅当控制台与文件均未通过 `setLogLevel` / `setConsoleLogLevel` / `setFileLogLevel` 显式设置时，同时作用于两者）、`ROBOT_QUAD_CONSOLE_LOG_LEVEL`、`ROBOT_QUAD_FILE_LOG_LEVEL`。
   *
   * @param level 日志等级。
   */
  static void setLogLevel(::robot::quad::LogLevel level);

  /**
   * @brief 单独设置控制台日志等级。
   *
   * 未显式配置时，控制台默认为 `LogLevel::WARN`。
   *
   * @param level 控制台日志等级。
   */
  static void setConsoleLogLevel(::robot::quad::LogLevel level);

  /**
   * @brief 单独设置文件日志等级。
   *
   * 未显式配置时，文件默认为 `LogLevel::DEBUG`。
   *
   * @param level 文件日志等级。
   */
  static void setFileLogLevel(::robot::quad::LogLevel level);

  /**
   * @brief 设置日志文件路径。
   *
   * 建议在创建首个 `Robot` 实例之前调用。该配置为进程级全局配置。传入空字符串时，
   * 默认路径 `./logs/quad_sdk.log`。
   *
   * @param file_path 日志文件路径。
   */
  static void setLogFilePath(const std::string& file_path);

  /**
   * @brief 注册全局日志回调。
   *
   * @param callback 日志回调；可为空。
   */
  static void setLogCallback(::robot::quad::LogCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_QUAD_ROBOT_HPP
