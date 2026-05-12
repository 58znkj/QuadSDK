#include <robot/quad/api.hpp>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

namespace quad = robot::quad;
using Clock = std::chrono::steady_clock;

constexpr auto kRobotConnectTimeout = std::chrono::seconds(10);
constexpr auto kCommandResultTimeout = std::chrono::seconds(60);
constexpr auto kLocalizationReadyTimeout = std::chrono::seconds(60);
constexpr auto kDefaultNavigationFinishTimeout = std::chrono::seconds(10);
constexpr auto kPollInterval = std::chrono::milliseconds(200);

struct Waypoint {
  std::string name;
  quad::Pose pose;
};

struct AppConfig {
  std::string robot_ip{"192.168.3.20"};
  std::string scene_name{"430"};
  std::vector<Waypoint> waypoints;
  std::chrono::milliseconds navigation_finish_timeout{
      kDefaultNavigationFinishTimeout};
  std::chrono::milliseconds advance_after{0};
  bool show_help{false};
};

struct WaypointTiming {
  std::string name;
  std::int64_t dispatch_to_finish_ms{0};
  std::int64_t finish_to_next_dispatch_ms{-1};
  bool advanced_to_next{false};
  quad::NavigationResult final_result;
  quad::NavigationState final_state_snapshot;
};

struct NavigationStateWaiter {
  std::mutex mutex;
  std::condition_variable condition;
  bool done{false};
  bool running_seen{false};
  quad::NavigationState state;
  Clock::time_point finish_time;
  quad::Subscription subscription;
};

quad::Pose makePose(float position_x,
                    float position_y,
                    float position_z,
                    float quaternion_w,
                    float quaternion_x,
                    float quaternion_y,
                    float quaternion_z) {
  quad::Pose pose;
  pose.position.x = position_x;
  pose.position.y = position_y;
  pose.position.z = position_z;
  pose.orientation.w = quaternion_w;
  pose.orientation.x = quaternion_x;
  pose.orientation.y = quaternion_y;
  pose.orientation.z = quaternion_z;
  return pose;
}

std::vector<Waypoint> defaultWaypoints() {
  return {
      {"point_1", makePose(1, 0, 0, 1.000f, 0.010f, 0.001f, -0.006f)},
      {"point_2", makePose(2, 0, 0, 1, 0.010f, 0.001f, -0.004f)},
      {"point_3", makePose(3, 0, 0, 0.872f, 0.017f, 0.004f, 0.489f)},
  };
}

std::vector<std::string> splitText(const std::string& text, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  std::istringstream stream(text);
  while (std::getline(stream, current, delimiter)) {
    parts.push_back(current);
  }
  if (!text.empty() && text.back() == delimiter) {
    parts.emplace_back();
  }
  return parts;
}

float parseFloatArgument(const std::string& text,
                         const std::string& field_name) {
  std::size_t consumed = 0;
  const float value = std::stof(text, &consumed);
  if (consumed != text.size()) {
    throw std::invalid_argument(field_name + " 不是有效浮点数：" + text);
  }
  return value;
}

std::chrono::milliseconds parseMillisecondsArgument(
    const std::string& text,
    const std::string& option_name,
    bool allow_zero) {
  std::size_t consumed = 0;
  const long long value = std::stoll(text, &consumed);
  if (consumed != text.size() || value < 0 || (!allow_zero && value == 0)) {
    throw std::invalid_argument(option_name + " 不是有效毫秒数：" + text);
  }
  return std::chrono::milliseconds(value);
}

Waypoint parseWaypointArgument(const std::string& text, std::size_t index) {
  const std::size_t colon = text.find(':');
  if (colon != std::string::npos) {
    Waypoint waypoint = parseWaypointArgument(text.substr(colon + 1), index);
    waypoint.name = text.substr(0, colon);
    if (waypoint.name.empty()) {
      throw std::invalid_argument("--point 名称不能为空：" + text);
    }
    return waypoint;
  }

  const std::vector<std::string> parts = splitText(text, ',');
  if (parts.size() != 7 && parts.size() != 8) {
    throw std::invalid_argument(
        "--point 格式应为 name,x,y,z,qw,qx,qy,qz 或 x,y,z,qw,qx,qy,qz：" +
        text);
  }

  const bool has_name = parts.size() == 8;
  const std::size_t offset = has_name ? 1 : 0;
  Waypoint waypoint;
  waypoint.name = has_name ? parts[0] : ("point_" + std::to_string(index));
  if (waypoint.name.empty()) {
    throw std::invalid_argument("--point 名称不能为空：" + text);
  }
  waypoint.pose = makePose(
      parseFloatArgument(parts[offset + 0], "x"),
      parseFloatArgument(parts[offset + 1], "y"),
      parseFloatArgument(parts[offset + 2], "z"),
      parseFloatArgument(parts[offset + 3], "qw"),
      parseFloatArgument(parts[offset + 4], "qx"),
      parseFloatArgument(parts[offset + 5], "qy"),
      parseFloatArgument(parts[offset + 6], "qz"));
  return waypoint;
}

bool consumeOptionValue(const std::string& arg,
                        const std::string& option_name,
                        int* index,
                        int argc,
                        char** argv,
                        std::string* value) {
  if (arg == option_name) {
    if (*index + 1 >= argc) {
      throw std::invalid_argument(option_name + " 缺少参数值");
    }
    *value = argv[++(*index)];
    return true;
  }

  const std::string prefix = option_name + "=";
  if (arg.rfind(prefix, 0) == 0) {
    *value = arg.substr(prefix.size());
    if (value->empty()) {
      throw std::invalid_argument(option_name + " 参数值不能为空");
    }
    return true;
  }
  return false;
}

void printUsage(const char* program_name) {
  std::cout
      << "用法：\n"
      << "  " << program_name
      << " [--robot-ip IP] [--scene NAME] [--point SPEC ...]\n"
      << "      [--advance-after-ms MS] [--finish-timeout-ms MS]\n\n"
      << "参数：\n"
      << "  --robot-ip IP              机器人 IP，默认 192.168.3.20\n"
      << "  --scene NAME               场景名称，默认 430\n"
      << "  --point SPEC               导航点，可重复。格式：\n"
      << "                             name,x,y,z,qw,qx,qy,qz\n"
      << "                             或 x,y,z,qw,qx,qy,qz\n"
      << "  --advance-after-ms MS      提前下发模式。当前点执行 MS 毫秒后，\n"
      << "                             若还未到终态，直接下发下一个点；0 表示关闭\n"
      << "  --finish-timeout-ms MS     单点等待终态超时，默认 "
      << kDefaultNavigationFinishTimeout.count() * 1000 << "\n"
      << "  --help                     打印帮助\n\n"
      << "兼容旧参数：\n"
      << "  " << program_name << " [robot_ip] [scene_name] [point ...]\n";
}

AppConfig parseArgs(int argc, char** argv) {
  AppConfig config;
  std::vector<Waypoint> parsed_waypoints;
  std::vector<std::string> positional;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    std::string value;
    if (arg == "--help" || arg == "-h") {
      config.show_help = true;
      return config;
    }
    if (consumeOptionValue(arg, "--robot-ip", &i, argc, argv, &value)) {
      config.robot_ip = value;
      continue;
    }
    if (consumeOptionValue(arg, "--scene", &i, argc, argv, &value)) {
      config.scene_name = value;
      continue;
    }
    if (consumeOptionValue(arg, "--point", &i, argc, argv, &value)) {
      parsed_waypoints.push_back(
          parseWaypointArgument(value, parsed_waypoints.size() + 1));
      continue;
    }
    if (consumeOptionValue(
            arg, "--advance-after-ms", &i, argc, argv, &value) ||
        consumeOptionValue(arg, "--early-next-ms", &i, argc, argv, &value)) {
      config.advance_after =
          parseMillisecondsArgument(value, "--advance-after-ms", true);
      continue;
    }
    if (consumeOptionValue(
            arg, "--finish-timeout-ms", &i, argc, argv, &value)) {
      config.navigation_finish_timeout =
          parseMillisecondsArgument(value, "--finish-timeout-ms", false);
      continue;
    }
    if (!arg.empty() && arg[0] == '-') {
      throw std::invalid_argument("未知参数：" + arg);
    }
    positional.push_back(arg);
  }

  if (!positional.empty()) {
    config.robot_ip = positional[0];
  }
  if (positional.size() >= 2) {
    config.scene_name = positional[1];
  }
  for (std::size_t i = 2; i < positional.size(); ++i) {
    parsed_waypoints.push_back(
        parseWaypointArgument(positional[i], parsed_waypoints.size() + 1));
  }

  if (parsed_waypoints.empty()) {
    config.waypoints = defaultWaypoints();
  } else {
    config.waypoints = std::move(parsed_waypoints);
  }
  if (config.waypoints.empty()) {
    throw std::invalid_argument("至少需要 1 个导航点");
  }
  return config;
}

std::string timestampText() {
  const auto now = std::chrono::system_clock::now();
  const auto time_value = std::chrono::system_clock::to_time_t(now);
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) %
                      1000;

  std::tm local_time{};
  localtime_r(&time_value, &local_time);

  std::ostringstream stream;
  stream << std::put_time(&local_time, "%F %T") << "."
         << std::setw(3) << std::setfill('0') << millis.count();
  return stream.str();
}

std::ostream& logInfo() {
  return std::cout << "[" << timestampText() << "] ";
}

std::ostream& logError() {
  return std::cerr << "[" << timestampText() << "] ";
}

std::int64_t elapsedMs(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
      .count();
}

const char* commandStatusText(quad::AsyncCommandStatus status) {
  switch (status) {
    case quad::AsyncCommandStatus::SUCCEEDED:
      return "SUCCEEDED";
    case quad::AsyncCommandStatus::TIMEOUT:
      return "TIMEOUT";
    case quad::AsyncCommandStatus::PUBLISH_FAILED:
      return "PUBLISH_FAILED";
    case quad::AsyncCommandStatus::CANCELLED:
      return "CANCELLED";
    case quad::AsyncCommandStatus::SHUTDOWN:
      return "SHUTDOWN";
    case quad::AsyncCommandStatus::FAILED:
      return "FAILED";
  }
  return "UNKNOWN";
}

const char* navigationStatusText(quad::NavigationStatus status) {
  switch (status) {
    case quad::NavigationStatus::IDLE:
      return "IDLE";
    case quad::NavigationStatus::RUNNING:
      return "RUNNING";
    case quad::NavigationStatus::FINISHED:
      return "FINISHED";
    case quad::NavigationStatus::CANCELLED:
      return "CANCELLED";
    case quad::NavigationStatus::FAILED:
      return "FAILED";
  }
  return "UNKNOWN";
}

bool isTerminalNavigationStatus(quad::NavigationStatus status) {
  return status == quad::NavigationStatus::FINISHED ||
         status == quad::NavigationStatus::CANCELLED ||
         status == quad::NavigationStatus::FAILED;
}

void printPose(const char* label, const quad::Pose& pose) {
  const quad::Pose2D pose_2d = quad::pose2DFromPose(pose);
  logInfo() << label
            << " x=" << pose_2d.x
            << " y=" << pose_2d.y
            << " yaw=" << pose_2d.yaw_degree << "\n";
}

void printNavigationState(const char* label,
                          const quad::NavigationState& state) {
  logInfo() << label
            << " status=" << navigationStatusText(state.status)
            << " task_id=" << state.task_id
            << " error="
            << quad::formatErrorCodeForLog(
                   state.error, quad::ErrorDescriptionLanguage::Zh)
            << " fresh=" << state.fresh << "\n";
  if (!state.error_message.empty()) {
    logInfo() << "  error_message=" << state.error_message << "\n";
  }
}

void printNavigationResult(const char* label,
                           const quad::NavigationResult& result) {
  logInfo() << label
            << " status=" << navigationStatusText(result.status)
            << " error="
            << quad::formatErrorCodeForLog(
                   result.error, quad::ErrorDescriptionLanguage::Zh)
            << " fresh=" << result.fresh << "\n";
}

void printNavigationStateEvent(const quad::NavigationState& state) {
  std::ostringstream stream;
  stream << "[" << timestampText() << "] "
         << "导航状态事件"
         << " status=" << navigationStatusText(state.status)
         << " task_id=" << state.task_id
         << " error="
         << quad::formatErrorCodeForLog(
                state.error, quad::ErrorDescriptionLanguage::Zh)
         << " fresh=" << state.fresh;
  if (!state.error_message.empty()) {
    stream << " error_message=" << state.error_message;
  }
  stream << "\n";
  std::cout << stream.str();
}

quad::NavigationResult navigationResultFromState(
    const quad::NavigationState& state) {
  quad::NavigationResult result;
  result.status = state.status;
  result.error = state.error;
  result.fresh = state.fresh;
  return result;
}

quad::SubscribeOptions freshChangedNoReplayOptions() {
  quad::SubscribeOptions options;
  options.replay_latest = false;
  options.only_changed = true;
  options.fresh_only = true;
  return options;
}

bool waitForCommandResult(
    const std::string& action_name,
    const std::function<quad::AsyncCommandId(quad::AsyncCommandCallback)>&
        submit,
    quad::AsyncCommandResult* result,
    Clock::time_point* dispatch_time = nullptr) {
  auto promise = std::make_shared<std::promise<quad::AsyncCommandResult>>();
  auto future = promise->get_future();

  const Clock::time_point issued_at = Clock::now();
  if (dispatch_time != nullptr) {
    *dispatch_time = issued_at;
  }

  const quad::AsyncCommandId command_id = submit(
      [promise](const quad::AsyncCommandResult& command_result) {
        try {
          promise->set_value(command_result);
        } catch (const std::future_error&) {
          // Ignore duplicate callbacks for the same command waiter.
        }
      });

  if (command_id == 0) {
    logError() << action_name << " 命令未入队\n";
    return false;
  }

  if (future.wait_for(kCommandResultTimeout) != std::future_status::ready) {
    logError() << action_name << " 等待命令结果超时"
               << " command_id=" << command_id << "\n";
    return false;
  }

  *result = future.get();
  const bool succeeded =
      result->status == quad::AsyncCommandStatus::SUCCEEDED &&
      result->error == quad::ErrorCode::SUCCESS;
  if (!succeeded) {
    std::ostream& stream = logError();
    stream << action_name << " 命令失败"
           << " command_id=" << result->command_id
           << " status=" << commandStatusText(result->status)
           << " error="
           << quad::formatErrorCodeForLog(
                  result->error, quad::ErrorDescriptionLanguage::Zh);
    if (!result->detail.empty()) {
      stream << " detail=" << result->detail;
    }
    stream << "\n";
    return false;
  }

  logInfo() << action_name << " 命令受理成功"
            << " command_id=" << result->command_id << "\n";
  return true;
}

bool refreshMapResources(quad::Robot& robot) {
  quad::AsyncCommandResult result;
  return waitForCommandResult(
      "刷新地图资源",
      [&robot](quad::AsyncCommandCallback callback) {
        return robot.nav().resource().refreshMapResources(std::move(callback));
      },
      &result);
}

bool sceneExists(const quad::MapResourceSnapshot& snapshot,
                 const std::string& scene_name) {
  for (const quad::SceneDetail& scene : snapshot.scenes) {
    if (scene.scene_name == scene_name) {
      return true;
    }
  }
  return false;
}

bool startLocalization(quad::Robot& robot,
                       const std::string& scene_name,
                       const quad::Pose& initial_pose) {
  printPose("重定位初始位姿", initial_pose);

  quad::AsyncCommandResult result;
  if (!waitForCommandResult(
          "启动重定位",
          [&robot, &scene_name, &initial_pose](
              quad::AsyncCommandCallback callback) {
            return robot.nav().mapping().startLocalization(
                scene_name, std::move(callback));
          },
          &result)) {
    return false;
  }

  auto deadline = Clock::now() + kLocalizationReadyTimeout;
  while (Clock::now() < deadline) {
    const quad::LocalizationState state =
        robot.nav().mapping().getLocalizationState();
    if (state.ready && state.fresh) {
      logInfo() << "定位已就绪"
                << " x=" << state.pose.position.x
                << " y=" << state.pose.position.y
                << " laser_quality=" << state.laser_quality << "\n";
      return true;
    }
    std::this_thread::sleep_for(kPollInterval);
  }

  logError() << "等待定位就绪超时\n";
  return false;
}

bool ensureNavigationMotionMode(quad::Robot& robot) {
  if (robot.nav().navigation().getMotionMode() == quad::MotionMode::Navigation) {
    logInfo() << "当前已是导航运动模式\n";
    return true;
  }

  quad::AsyncCommandResult result;
  return waitForCommandResult(
      "切换导航运动模式",
      [&robot](quad::AsyncCommandCallback callback) {
        return robot.nav().navigation().setMotionMode(
            quad::MotionMode::Navigation, std::move(callback));
      },
      &result);
}

bool waitForNavigationFinished(quad::Robot& robot,
                               const std::shared_ptr<NavigationStateWaiter>&
                                   waiter,
                               std::chrono::milliseconds timeout,
                               quad::NavigationState* final_state,
                               Clock::time_point* finish_time,
                               bool log_timeout = true) {
  if (!waiter || !waiter->subscription.valid()) {
    logError() << "导航状态订阅未建立\n";
    return false;
  }

  std::unique_lock<std::mutex> lock(waiter->mutex);
  const bool ready = waiter->condition.wait_until(
      lock, Clock::now() + timeout, [&waiter] {
        return waiter->done;
      });
  if (ready) {
    if (final_state != nullptr) {
      *final_state = waiter->state;
    }
    if (finish_time != nullptr) {
      *finish_time = waiter->finish_time;
    }
    waiter->subscription.unsubscribe();
    return true;
  }

  waiter->subscription.unsubscribe();
  if (final_state != nullptr) {
    *final_state = robot.nav().navigation().getNavigationState();
  }
  if (log_timeout) {
    logError() << "等待导航任务执行完成超时\n";
  }
  return false;
}

std::shared_ptr<NavigationStateWaiter> createNavigationStateWaiter(
    quad::Robot& robot) {
  auto waiter = std::make_shared<NavigationStateWaiter>();
  waiter->subscription =
      robot.nav().navigation().registerNavigationStateEvent(
          [waiter](const quad::NavigationState& state) {
            if (state.fresh) {
              printNavigationStateEvent(state);
            }
            if (!state.fresh) {
              return;
            }

            {
              std::lock_guard<std::mutex> lock(waiter->mutex);
              if (waiter->done) {
                return;
              }
              if (state.status == quad::NavigationStatus::RUNNING) {
                waiter->running_seen = true;
                return;
              }
              if (!waiter->running_seen ||
                  !isTerminalNavigationStatus(state.status)) {
                return;
              }
              waiter->done = true;
              waiter->state = state;
              waiter->finish_time = Clock::now();
            }
            waiter->condition.notify_one();
          },
          freshChangedNoReplayOptions());
  return waiter;
}

void printTimingSummary(const std::vector<WaypointTiming>& timings) {
  std::cout << "\n";
  logInfo() << "定点导航耗时统计（单位：ms）\n";
  for (const WaypointTiming& timing : timings) {
    std::ostream& stream = logInfo();
    stream << timing.name
           << (timing.advanced_to_next ? " dispatch_to_advance="
                                        : " dispatch_to_finish=")
           << timing.dispatch_to_finish_ms
           << (timing.advanced_to_next ? " advance_to_next_dispatch="
                                        : " finish_to_next_dispatch=");
    if (timing.finish_to_next_dispatch_ms >= 0) {
      stream << timing.finish_to_next_dispatch_ms;
    } else {
      stream << "N/A";
    }
    stream << " final_status="
           << navigationStatusText(timing.final_result.status)
           << " advanced=" << (timing.advanced_to_next ? "true" : "false")
           << " task_id=" << timing.final_state_snapshot.task_id << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const AppConfig config = parseArgs(argc, argv);
    if (config.show_help) {
      printUsage(argv[0]);
      return 0;
    }

    const quad::Pose relocalization_pose =
        quad::poseFromXYYawDegrees(0.0f, 0.0f, 0.0f);

    quad::RobotOptions options;
    options.robot_ip = config.robot_ip;
    quad::Robot robot(options);

    auto finish = [&robot](int ret_code) {
      if (robot.isConnected()) {
        robot.disconnect();
        logInfo() << "机器人断开连接\n";
      }
      return ret_code;
    };

    if (!robot.connect(kRobotConnectTimeout)) {
      logError() << "机器人连接失败：" << robot.getRobotIp() << "\n";
      return finish(1);
    }
    logInfo() << "机器人连接成功：" << robot.getRobotIp() << "\n";

    if (!refreshMapResources(robot)) {
      return finish(1);
    }

    const quad::MapResourceSnapshot map_resource =
        robot.nav().resource().getMapResources();
    if (!sceneExists(map_resource, config.scene_name)) {
      logError() << "场景不存在：" << config.scene_name << "\n";
      return finish(1);
    }
    logInfo() << "场景存在：" << config.scene_name << "\n";

    if (!startLocalization(robot, config.scene_name, relocalization_pose)) {
      return finish(1);
    }

    if (!ensureNavigationMotionMode(robot)) {
      return finish(1);
    }

    std::vector<WaypointTiming> timings;
    Clock::time_point previous_finish_time;
    bool has_previous_finish_time = false;
    if (config.advance_after.count() > 0 && config.waypoints.size() > 1) {
      logInfo() << "提前下发模式开启 advance_after_ms="
                << config.advance_after.count() << "\n";
    }

    for (std::size_t waypoint_index = 0;
         waypoint_index < config.waypoints.size();
         ++waypoint_index) {
      const Waypoint& waypoint = config.waypoints[waypoint_index];
      const bool has_next_waypoint =
          waypoint_index + 1 < config.waypoints.size();

      std::shared_ptr<NavigationStateWaiter> state_waiter =
          createNavigationStateWaiter(robot);
      if (!state_waiter->subscription.valid()) {
        logError() << "导航状态事件订阅失败\n";
        printTimingSummary(timings);
        return finish(1);
      }

      printPose("下发导航点", waypoint.pose);

      quad::AsyncCommandResult start_result;
      Clock::time_point dispatch_time;
      if (!waitForCommandResult(
              "启动定点导航 " + waypoint.name,
              [&robot, &config, &waypoint](
                  quad::AsyncCommandCallback callback) {
                return robot.nav().navigation().startNavigation(
                    config.scene_name, waypoint.pose, std::move(callback));
              },
              &start_result,
              &dispatch_time)) {
        state_waiter->subscription.unsubscribe();
        printTimingSummary(timings);
        return finish(1);
      }

      if (has_previous_finish_time && !timings.empty()) {
        timings.back().finish_to_next_dispatch_ms =
            elapsedMs(previous_finish_time, dispatch_time);
        logInfo() << (timings.back().advanced_to_next
                          ? "上一任务提前切换到当前指令下发间隔="
                          : "上一任务完成到当前指令下发间隔=")
                  << timings.back().finish_to_next_dispatch_ms << " ms\n";
      }

      quad::NavigationState final_state;
      Clock::time_point finish_time;
      const bool advance_this_point =
          config.advance_after.count() > 0 && has_next_waypoint;
      const bool terminal_ready = waitForNavigationFinished(
          robot,
          state_waiter,
          advance_this_point ? config.advance_after
                             : config.navigation_finish_timeout,
          &final_state,
          &finish_time,
          !advance_this_point);
      if (!terminal_ready && advance_this_point) {
        finish_time = Clock::now();
        const quad::NavigationResult final_result =
            navigationResultFromState(final_state);
        printNavigationState("提前切换时导航状态", final_state);
        printNavigationResult("提前切换时导航结果", final_result);

        WaypointTiming timing;
        timing.name = waypoint.name;
        timing.dispatch_to_finish_ms = elapsedMs(dispatch_time, finish_time);
        timing.advanced_to_next = true;
        timing.final_result = final_result;
        timing.final_state_snapshot = final_state;
        timings.push_back(timing);

        logInfo() << waypoint.name << " 执行 "
                  << timing.dispatch_to_finish_ms
                  << " ms 后提前下发下一个导航点\n";

        previous_finish_time = finish_time;
        has_previous_finish_time = true;
        continue;
      }

      if (!terminal_ready) {
        const quad::NavigationResult final_result =
            navigationResultFromState(final_state);
        printNavigationState("导航最终状态", final_state);
        printNavigationResult("导航最终结果", final_result);
        printTimingSummary(timings);
        return finish(1);
      }
      state_waiter->subscription.unsubscribe();

      const quad::NavigationResult final_result =
          navigationResultFromState(final_state);
      printNavigationResult("导航最终结果", final_result);
      printNavigationState("导航最终状态", final_state);

      WaypointTiming timing;
      timing.name = waypoint.name;
      timing.dispatch_to_finish_ms = elapsedMs(dispatch_time, finish_time);
      timing.final_result = final_result;
      timing.final_state_snapshot = final_state;
      timings.push_back(timing);

      logInfo() << waypoint.name << " 指令下发到任务完成间隔="
                << timing.dispatch_to_finish_ms << " ms\n";

      previous_finish_time = finish_time;
      has_previous_finish_time = true;

      if (final_result.status != quad::NavigationStatus::FINISHED ||
          final_result.error != quad::ErrorCode::SUCCESS) {
        logError() << waypoint.name << " 未成功完成，停止下发后续导航点\n";
        printTimingSummary(timings);
        return finish(1);
      }
    }

    printTimingSummary(timings);
    return finish(0);
  } catch (const std::exception& e) {
    logError() << "连续定点导航示例执行失败：" << e.what() << "\n";
    return 1;
  }
}
