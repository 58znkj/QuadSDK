#ifndef ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP
#define ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP

/**
 * @file nav/types.hpp
 * @brief Quad SDK 导航、建图与地图资源相关类型定义。
 */

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
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

/** @brief SLAM 工作模式。 */
enum class SLAMWorkMode {
    IDLE = 0,          /**< 空闲。 */
    MAPPING = 1,       /**< 建图中。 */
    SAVING = 2,        /**< 保存地图中。 */
    RELOCATING = 3,    /**< 重定位中。 */
    LOCALIZING = 4     /**< 定位中。 */
};

/** @brief 建图场景类型。 */
enum class MappingSceneType {
    INDOOR = 1,            /**< 室内场景。 */
    OUTDOOR_NORMAL = 2,    /**< 常规室外场景。 */
    OUTDOOR_OPEN = 3       /**< 开阔室外场景。 */
};

/** @brief SLAM 错误码。 */
enum class SLAMErrorCode {
    NORMAL = 0,              /**< 正常。 */
    UNABLE_START = 1,        /**< 无法启动。 */
    NO_IMU = 2,              /**< 缺少 IMU 数据。 */
    NO_LIDAR = 3,            /**< 缺少雷达数据。 */
    LIDAR_INCOMPLETE = 4,    /**< 雷达数据不完整。 */
    MAP_NOT_FOUND = 5,       /**< 地图不存在。 */
    LOCALIZE_ERROR = 6,      /**< 重定位失败。 */
    SLAM_COLLAPSE = 7,       /**< SLAM 崩溃。 */
    MAPPING_FAIL = 8,        /**< 建图失败。 */
    LIDAR_ERROR = 9,         /**< 雷达设备异常。 */
    IMU_ERROR = 10,          /**< IMU 设备异常。 */
    GNSS_ERROR = 11,         /**< GNSS 设备异常。 */
    SYNC_ERROR = 12          /**< 传感器同步异常。 */
};

/** @brief 建图路径点。 */
struct MappingPathPoint {
    double x{0};    /**< X 坐标。 */
    double y{0};    /**< Y 坐标。 */
    double z{0};    /**< Z 坐标。 */
};

/** @brief 定位结果。 */
struct LocalizationInfo {
    float position_x{0};      /**< X 坐标（米）。 */
    float position_y{0};      /**< Y 坐标（米）。 */
    float position_z{0};      /**< Z 坐标（米）。 */
    float orientation_w{1};   /**< 四元数 w 分量。 */
    float orientation_x{0};   /**< 四元数 x 分量。 */
    float orientation_y{0};   /**< 四元数 y 分量。 */
    float orientation_z{0};   /**< 四元数 z 分量。 */
    float laser_quality{0};   /**< 激光质量评分。 */
};

/** @brief 当前导航状态快照。 */
struct NavigationState {
    NavigationStatus status{NavigationStatus::IDLE};    /**< 当前导航任务状态。 */
    uint32_t         task_id{0};                        /**< 当前导航任务 ID。 */
    uint32_t         task_mode{0};                      /**< 当前导航任务模式（协议层）。 */
    uint32_t         output_status{0};                  /**< 导航输出状态码（协议层）。 */
    uint32_t         current_map_index{0};              /**< 当前地图索引。 */
    ErrorCode        error{ErrorCode::SUCCESS};         /**< 统一错误码，由 SDK 映射自协议层。 */
    std::string      error_message;                     /**< 错误描述文本，用于排障。 */
    bool             fresh{false};                      /**< 当前连接周期内是否已收到有效上报。 */
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

/** @brief 轨迹录制结果。 */
enum class RecordResult {
    POINT_ADDED = 0,    /**< 新增了一个录制点。 */
    SUCCESS = 1,        /**< 录制成功。 */
    FAIL = 2,           /**< 录制失败。 */
    STARTING = 3        /**< 录制启动中。 */
};

/** @brief SLAM 状态快照。 */
struct SlamState {
    SLAMWorkMode  mode{SLAMWorkMode::IDLE};         /**< 当前 SLAM 工作模式。 */
    SLAMErrorCode error{SLAMErrorCode::NORMAL};     /**< SLAM 错误码。 */
    bool          is_mapping{false};                /**< 是否处于建图中。 */
    bool          fresh{false};                     /**< 当前连接周期内是否已收到有效上报。 */
};

/** @brief 定位状态快照。 */
struct LocalizationState {
    LocalizationInfo info;          /**< 定位位姿与激光质量。 */
    bool             ready{false};  /**< 定位是否就绪。 */
    bool             fresh{false};  /**< 当前连接周期内是否已收到有效上报。 */
};

/** @brief 建图进度状态。 */
struct MappingProgressState {
    std::optional<MappingPathPoint> latest_path_point;  /**< 最新建图路径点。 */
    bool                            fresh{false};       /**< 建图路径数据是否新鲜。 */
};

/** @brief 录制状态快照。 */
struct RecordingState {
    RecordResult result{RecordResult::FAIL};     /**< 最近一次录制结果。 */
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

/** @brief SLAM 状态事件回调。 */
using SlamStateEventCallback = std::function<void(const SlamState&)>;
/** @brief 定位状态事件回调。 */
using LocalizationStateEventCallback =
    std::function<void(const LocalizationState&)>;
/** @brief 建图路径点事件回调。 */
using MappingPathPointEventCallback =
    std::function<void(const std::optional<MappingPathPoint>&)>;
/** @brief 建图进度状态事件回调。 */
using MappingProgressEventCallback =
    std::function<void(const MappingProgressState&)>;
/** @brief 导航状态事件回调。 */
using NavigationStateEventCallback = std::function<void(const NavigationState&)>;
/** @brief 导航结果事件回调。 */
using NavigationResultEventCallback = std::function<void(const NavigationResult&)>;
/** @brief 暂停状态事件回调。 */
using PauseStateEventCallback = std::function<void(const PauseState&)>;
/** @brief 录制状态事件回调。 */
using RecordingStateEventCallback =
    std::function<void(const RecordingState&)>;
/** @brief 地图资源事件回调。 */
using MapResourceEventCallback =
    std::function<void(const MapResourceEvent&)>;
/** @brief 地图传输完成回调。 */
using MapTransferCallback = std::function<void(ErrorCode)>;

}  // namespace quad
}  // namespace robot

#endif  // ROBOT_PUBLIC_QUAD_NAV_TYPES_HPP
