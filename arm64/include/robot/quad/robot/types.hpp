#ifndef ROBOT_PUBLIC_QUAD_ROBOT_TYPES_HPP
#define ROBOT_PUBLIC_QUAD_ROBOT_TYPES_HPP

/**
 * @file robot/types.hpp
 * @brief Quad SDK 机器人级配置与日志类型定义。
 */

#include <functional>
#include <string>

namespace robot {
namespace quad {

/** @brief 创建机器人实例时的连接与心跳配置。 */
struct RobotOptions {
    std::string robot_ip;                    /**< 机器人 IP 地址。 */
    int         mqtt_port = 11058;           /**< MQTT 端口。 */
};

/** @brief 日志等级。 */
enum class LogLevel {
    DEBUG = 0,    /**< 调试日志。 */
    INFO = 1,     /**< 信息日志。 */
    WARN = 2,     /**< 警告日志。 */
    ERROR = 3     /**< 错误日志。 */
};

/** @brief 日志输出回调。 */
using LogCallback = std::function<void(LogLevel,
                                       const std::string& logger_name,
                                       const std::string& message)>;

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_ROBOT_TYPES_HPP
