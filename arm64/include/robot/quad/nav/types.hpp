#ifndef ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP
#define ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP

/**
 * @file nav/types.hpp
 * @brief Quad SDK 导航、定位与地图资源相关类型定义。
 */

#include <chrono>
#include <cstdint>
#include <functional>
#include <robot/quad/common/types.hpp>
#include <string>
#include <vector>

namespace robot {
namespace quad {

/** @brief 地图文件传输选项。 */
struct MapTransferOptions {
    std::chrono::seconds transfer_timeout{60};  /**< 文件传输总超时。 */
    std::chrono::seconds connect_timeout{10};   /**< FTP 连接超时。 */
};

/** @brief 导航任务状态。 */
enum class NavigationStatus {
    IDLE = 0,         /**< 空闲。 */
    RUNNING = 1,      /**< 执行中。 */
    FINISHED = 2,     /**< 已完成。 */
    CANCELLED = 3,    /**< 已取消。 */
    FAILED = 4        /**< 执行失败。 */
};

/** @brief 导航任务模式。 */
enum class NavigationTaskMode : uint32_t {
    POSE = 0,            /**< 定点导航。 */
    PATH = 1,            /**< 循迹导航。 */
    UNKNOWN = 0xFFFFFFFF /**< 未知模式。 */
};

/** @brief 导航输出状态。 */
enum class NavigationOutputStatus : uint32_t {
    NONE = 0,            /**< 无任务状态。 */
    FINISHED = 1,        /**< 导航任务完成。 */
    RUNNING = 2,         /**< 导航任务正在执行。 */
    CANCELLED = 3,       /**< 导航任务取消。 */
    FAILED = 4,          /**< 导航任务失败。 */
    UNKNOWN = 0xFFFFFFFF /**< 未知输出状态。 */
};

/** @brief 当前导航状态快照。 */
struct NavigationState {
    NavigationStatus       status{NavigationStatus::IDLE};          /**< 当前导航任务状态。 */
    uint32_t               task_id{0};                              /**< 当前导航任务 ID。 */
    NavigationTaskMode     task_mode{NavigationTaskMode::UNKNOWN};  /**< 当前导航任务模式。 */
    NavigationOutputStatus output_status{NavigationOutputStatus::NONE};    /**< 导航输出状态。 */
    uint32_t               current_map_index{0};                    /**< 当前地图索引。 */
    ErrorCode              error{ErrorCode::SUCCESS};               /**< 统一错误码，由 SDK 映射自协议层。 */
    std::string            error_message;                           /**< 错误描述文本，用于排障。 */
    bool                   fresh{false};                            /**< 当前连接周期内是否已收到有效上报。 */
};

/** @brief 最近一次导航终态结果。 */
struct NavigationResult {
    NavigationStatus status{NavigationStatus::IDLE};    /**< 最近一次导航终态。 */
    ErrorCode        error{ErrorCode::SUCCESS};         /**< 统一错误码，由 SDK 映射自协议层。 */
    bool             fresh{false};                      /**< 当前连接周期内是否已收到有效终态上报。 */
};

/** @brief 导航暂停状态。 */
struct PauseState {
    bool paused{false};     /**< 是否处于暂停状态。 */
    bool fresh{false};      /**< 当前连接周期内是否已收到有效上报。 */
};

/** @brief 子场景信息。 */
struct SceneInfo {
    uint32_t    sub_scene_id{0};    /**< 子场景 ID。 */
    std::string yam_filename;       /**< 地图 YAML 文件名。 */
    std::string pgm_filename;       /**< 地图 PGM 文件名。 */
};

/** @brief 场景详情。 */
struct SceneDetail {
    uint32_t               scene_id{0};     /**< 场景 ID。 */
    std::string            scene_name;      /**< 场景名称。 */
    std::vector<SceneInfo> sub_scenes;      /**< 子场景列表。 */
};

/** @brief 导航路点。 */
struct NavigationPoint {
    int32_t point_id{0};        /**< 路点 ID。 */
    int32_t sub_scene_id{0};    /**< 所属子场景 ID。 */
    Pose    pose;               /**< 路点位姿。 */
};

/** @brief 导航路径。 */
struct NavigationPath {
    int32_t           path_id{0};           /**< 路径 ID。 */
    std::string       path_name;            /**< 路径名称。 */
    int32_t           start_point_id{0};    /**< 起始路点 ID。 */
    int32_t           end_point_id{0};      /**< 终止路点 ID。 */
    std::vector<Pose> points;               /**< 路径点列表。 */
};

/** @brief 场景中的导航拓扑。 */
struct NavigationTrajectory {
    int32_t                       graph_id{0};      /**< 轨迹图 ID。 */
    std::string                   scene_name;       /**< 所属场景名称。 */
    std::vector<NavigationPoint>  waypoints;        /**< 路点列表。 */
    std::vector<NavigationPath>   paths;            /**< 路径列表。 */
};

/** @brief 定位状态快照。 */
struct LocalizationState {
    Pose  pose;                 /**< 定位位姿。 */
    float laser_quality{0};     /**< 激光质量评分。 */
    bool  ready{false};         /**< 定位是否就绪。 */
    bool  fresh{false};         /**< 当前连接周期内是否已收到有效上报。 */
};

/** @brief 地图资源缓存快照。 */
struct MapResourceSnapshot {
    std::vector<SceneDetail>          scenes;                       /**< 场景列表缓存。 */
    std::vector<NavigationTrajectory> trajectories;                 /**< 轨迹列表缓存。 */
    bool                              scenes_valid{false};          /**< 场景缓存是否有效。 */
    bool                              trajectories_valid{false};    /**< 轨迹缓存是否有效。 */
    uint64_t                          scene_resource_version{0};    /**< 场景资源版本号。 */
    uint64_t                          trajectory_resource_version{0}; /**< 轨迹资源版本号。 */
    uint64_t                          connection_generation{0};     /**< 当前连接代际。 */
};

/** @brief 地图资源事件类型。 */
enum class MapResourceEventType {
    SCENES_UPDATED = 0,         /**< 场景列表已更新。 */
    TRAJECTORIES_UPDATED = 1,   /**< 轨迹列表已更新。 */
    SCENE_REMOVED = 2,          /**< 场景已删除。 */
    TRAJECTORY_REMOVED = 3,     /**< 轨迹已删除。 */
    PATH_REMOVED = 4,           /**< 路径已删除。 */
    PATH_RENAMED = 5,           /**< 路径已重命名。 */
};

/** @brief 地图资源事件。 */
struct MapResourceEvent {
    MapResourceEventType type{MapResourceEventType::SCENES_UPDATED};    /**< 事件类型。 */
    std::string          scene_name;    /**< 关联的场景名称。 */
    std::string          path_name;     /**< 关联的路径名称（仅路径相关事件有值）。 */
};

/** @brief 定位状态事件回调。 */
using LocalizationStateEventCallback =
    std::function<void(const LocalizationState&)>;
/** @brief 导航状态事件回调。 */
using NavigationStateEventCallback = std::function<void(const NavigationState&)>;
/** @brief 导航结果事件回调。 */
using NavigationResultEventCallback = std::function<void(const NavigationResult&)>;
/** @brief 暂停状态事件回调。 */
using PauseStateEventCallback = std::function<void(const PauseState&)>;
/** @brief 地图资源事件回调。 */
using MapResourceEventCallback =
    std::function<void(const MapResourceEvent&)>;
/** @brief 地图传输完成回调。 */
using MapTransferCallback = std::function<void(ErrorCode)>;

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP
