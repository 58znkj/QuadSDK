#ifndef ROBOT_PUBLIC_QUAD_ERROR_HPP
#define ROBOT_PUBLIC_QUAD_ERROR_HPP

/**
 * @file error.hpp
 * @brief Quad SDK 统一错误码与错误详情结构定义。
 */

#include <robot/quad/export.hpp>

#include <string>

namespace robot {
namespace quad {

/**
 * @brief 统一错误码。
 *
 * 枚举值当前覆盖 Quad facade 已公开的导航、定位/SLAM 与 SDK 运行时错误，
 * 以便上层仅通过一个错误域完成判断与日志记录。
 */
enum class ErrorCode {
    SUCCESS = 0,                                 /**< 成功。 */

    NAV_UNKNOWN_TASK = 10,                       /**< 导航任务不存在或不可识别。 */
    NAV_STOP_BARRIER = 11,                       /**< 导航因障碍物停止。 */
    NAV_MAP_CHANGE_FAIL = 12,                    /**< 地图切换失败。 */
    NAV_MAP_INDEX_ERROR = 13,                    /**< 地图索引无效。 */
    NAV_STOP_BARRIER_ON_STAIR = 14,              /**< 楼梯场景因障碍物停止。 */
    NAV_PREEMPTED = 20,                          /**< 导航任务被抢占。 */
    NAV_TASK_CANCELLED = 21,                     /**< 导航任务被本体取消。 */
    NAV_UNKNOWN_ERROR = 99,                      /**< 未知导航错误。 */
    NAV_OUT_OF_TRACK = 30,                       /**< 机器人偏离轨迹。 */
    NAV_NAV_COLLAPSE = 31,                       /**< 导航解算异常。 */
    NAV_FAR_AWAY_PATH = 32,                      /**< 目标路径距离过远。 */
    NAV_UWB_DATA_GET_SUCCEED = 33,               /**< UWB 数据获取成功。 */
    NAV_UWB_DATA_GET_FAIL = 34,                  /**< UWB 数据获取失败。 */
    NAV_TF_ERROR = 35,                           /**< 坐标变换异常。 */
    NAV_PAT_EXCEEDED = 36,                       /**< 规划路径超出限制。 */
    NAV_TASK_PATH_INVALID = 37,                  /**< 导航任务路径无效。 */

    SLAM_NORMAL = 300,                           /**< SLAM 正常。 */
    SLAM_UNABLE_START = 301,                     /**< SLAM 无法启动。 */
    SLAM_NO_IMU = 302,                           /**< 缺少 IMU 数据。 */
    SLAM_NO_LIDAR = 303,                         /**< 缺少激光雷达数据。 */
    SLAM_LIDAR_INCOMPLETE = 304,                 /**< 雷达数据不完整。 */
    SLAM_MAP_NOFOUND = 305,                      /**< 地图未找到。 */
    SLAM_LOCALIZE_ERROR = 306,                   /**< 重定位失败。 */
    SLAM_COLLAPSE = 307,                         /**< SLAM 崩溃。 */
    SLAM_MAPPING_FAIL = 308,                     /**< 建图失败。 */
    SLAM_LIDAR_ERROR = 309,                      /**< 雷达设备异常。 */
    SLAM_IMU_ERROR = 310,                        /**< IMU 设备异常。 */
    SLAM_GNSS_ERROR = 311,                       /**< GNSS 设备异常。 */
    SLAM_SYNC_ERROR = 312,                       /**< 传感器同步异常。 */

    SDK_COMMAND_TIMEOUT = 600,                   /**< SDK 命令等待超时。 */
    SDK_MQTT_PUBLISH_FAILED = 601,               /**< SDK MQTT 发布失败。 */
    SDK_TRANSPORT_DISCONNECTED = 602,            /**< SDK 传输链路断开。 */
    SDK_ROBOT_OFFLINE = 603,                     /**< 机器人离线。 */
    SDK_COMMAND_CANCELLED = 604,                 /**< SDK 命令被取消。 */
    SDK_RUNTIME_SHUTDOWN = 605,                  /**< SDK 运行时已关闭。 */
    SDK_COMMAND_FAILED = 606,                    /**< SDK 命令执行失败。 */
    SDK_RESOURCE_NOT_FOUND = 607,                /**< SDK 所需资源不存在。 */
    SDK_IO_ERROR = 608,                          /**< SDK I/O 异常。 */
    SDK_PROTOCOL_UNKNOWN_CODE = 609,             /**< 收到未知协议错误码。 */
    SDK_COMMAND_SUPERSEDED = 610,                /**< SDK 命令被同通道新命令替换。 */
    SDK_INTERNAL_ERROR = 699,                    /**< SDK 内部错误。 */
};

/** @brief 错误详情。 */
struct ErrorItem {
    ErrorCode   code{ErrorCode::SUCCESS};    /**< 统一错误码。 */
    std::string name;                        /**< 错误名称。 */
    std::string description_en;              /**< 英文描述。 */
    std::string description_cn;              /**< 中文描述。 */
    std::string detail;                      /**< 补充诊断信息。 */
};

/** @brief 错误描述语言。 */
enum class ErrorDescriptionLanguage {
    En,    /**< 英文描述。 */
    Zh,    /**< 中文描述。 */
};

/**
 * @brief 以日志友好格式输出错误码。
 *
 * 输出格式：`< ErrorCode | NAME | DESCRIPTION >`
 *
 * @param code 统一错误码。
 * @param lang 描述语言，默认英文。
 * @return 日志格式化后的错误字符串。
 */
ROBOT_QUAD_API std::string formatErrorCodeForLog(
    ErrorCode code,
    ErrorDescriptionLanguage lang = ErrorDescriptionLanguage::En);

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_ERROR_HPP
