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
constexpr auto kNavigationFinishTimeout = std::chrono::minutes(10);
constexpr auto kNavigationSettleDelay = std::chrono::milliseconds(500);
constexpr auto kPollInterval = std::chrono::milliseconds(200);

struct Waypoint {
  std::string name;
  quad::Pose pose;
};

struct WaypointTiming {
  std::string name;
  std::int64_t dispatch_to_finish_ms{0};
  std::int64_t finish_to_next_dispatch_ms{-1};
  quad::NavigationResult final_result;
  quad::NavigationState final_state_snapshot;
};

struct NavigationResultWaiter {
  std::mutex mutex;
  std::condition_variable condition;
  bool done{false};
  quad::NavigationResult result;
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
  return 
    status == quad::NavigationStatus::FINISHED ||
    status == quad::NavigationStatus::CANCELLED ||
    status == quad::NavigationStatus::FAILED;
  // status == quad::NavigationStatus::IDLE ||

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
                << " x=" << state.info.position_x
                << " y=" << state.info.position_y
                << " laser_quality=" << state.info.laser_quality << "\n";
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

bool waitForNavigationResult(quad::Robot& robot,
                             const std::shared_ptr<NavigationResultWaiter>&
                                 waiter,
                             quad::NavigationResult* final_result,
                             Clock::time_point* finish_time) {
  if (!waiter || !waiter->subscription.valid()) {
    logError() << "导航结果订阅未建立\n";
    return false;
  }

  std::unique_lock<std::mutex> lock(waiter->mutex);
  const bool ready = waiter->condition.wait_until(
      lock, Clock::now() + kNavigationFinishTimeout, [&waiter] {
        return waiter->done;
      });
  if (ready) {
    if (final_result != nullptr) {
      *final_result = waiter->result;
    }
    if (finish_time != nullptr) {
      *finish_time = waiter->finish_time;
    }
    waiter->subscription.unsubscribe();
    return true;
  }

  waiter->subscription.unsubscribe();
  if (final_result != nullptr) {
    *final_result = robot.nav().navigation().getNavigationResult();
  }
  logError() << "等待导航任务执行完成超时\n";
  return false;
}

std::shared_ptr<NavigationResultWaiter> createNavigationResultWaiter(
    quad::Robot& robot) {
  auto waiter = std::make_shared<NavigationResultWaiter>();
  waiter->subscription =
      robot.nav().navigation().registerNavigationResultEvent(
          [waiter](const quad::NavigationResult& result) {
            if (!result.fresh ||
                !isTerminalNavigationStatus(result.status)) {
              return;
            }

            {
              std::lock_guard<std::mutex> lock(waiter->mutex);
              if (waiter->done) {
                return;
              }
              waiter->done = true;
              waiter->result = result;
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
           << " dispatch_to_finish=" << timing.dispatch_to_finish_ms
           << " finish_to_next_dispatch=";
    if (timing.finish_to_next_dispatch_ms >= 0) {
      stream << timing.finish_to_next_dispatch_ms;
    } else {
      stream << "N/A";
    }
    stream << " final_status="
           << navigationStatusText(timing.final_result.status)
           << " task_id=" << timing.final_state_snapshot.task_id << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string robot_ip =
        argc > 1 ? argv[1] : std::string("192.168.3.20");
    const std::string scene_name =
        argc > 2 ? argv[2] : std::string("430");

    const quad::Pose relocalization_pose =
        quad::poseFromXYYawDegrees(0.0f, 0.0f, 0.0f);

    const std::vector<Waypoint> waypoints = {
        {"point_1",
         makePose(1,
                  0,
                  0,
                  1.000f,
                  0.010f,
                  0.001f,
                  -0.006f)},
        {"point_2",
         makePose(1.5,
                  0,
                  0,
                  1,
                  0.010f,
                  0.001f,
                  -0.004f)},
        {"point_3",
         makePose(2,
                  0,
                  0,
                  0.872f,
                  0.017f,
                  0.004f,
                  0.489f)},
    };

    quad::RobotOptions options;
    options.robot_ip = robot_ip;
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
    if (!sceneExists(map_resource, scene_name)) {
      logError() << "场景不存在：" << scene_name << "\n";
      return finish(1);
    }
    logInfo() << "场景存在：" << scene_name << "\n";

    if (!startLocalization(robot, scene_name, relocalization_pose)) {
      return finish(1);
    }

    if (!ensureNavigationMotionMode(robot)) {
      return finish(1);
    }

    std::vector<WaypointTiming> timings;
    Clock::time_point previous_finish_time;
    bool has_previous_finish_time = false;

    for (const Waypoint& waypoint : waypoints) {
      if (has_previous_finish_time) {
        // logInfo() << "等待导航状态稳定 "
        //           << kNavigationSettleDelay.count() << " ms\n";
        // std::this_thread::sleep_for(kNavigationSettleDelay);
      }

      std::shared_ptr<NavigationResultWaiter> result_waiter =
          createNavigationResultWaiter(robot);
      if (!result_waiter->subscription.valid()) {
        logError() << "导航结果事件订阅失败\n";
        printTimingSummary(timings);
        return finish(1);
      }

      quad::Subscription navigation_state_subscription =
          robot.nav().navigation().registerNavigationStateEvent(
              [](const quad::NavigationState& state) {
                if (state.fresh) {
                  printNavigationStateEvent(state);
                }
              },
              freshChangedNoReplayOptions());
      if (!navigation_state_subscription.valid()) {
        logError() << "导航状态事件订阅失败\n";
        result_waiter->subscription.unsubscribe();
        printTimingSummary(timings);
        return finish(1);
      }

      printPose("下发导航点", waypoint.pose);

      quad::AsyncCommandResult start_result;
      Clock::time_point dispatch_time;
      if (!waitForCommandResult(
              "启动定点导航 " + waypoint.name,
              [&robot, &scene_name, &waypoint](
                  quad::AsyncCommandCallback callback) {
                return robot.nav().navigation().startNavigation(
                    scene_name, waypoint.pose, std::move(callback));
              },
              &start_result,
              &dispatch_time)) {
        result_waiter->subscription.unsubscribe();
        navigation_state_subscription.unsubscribe();
        printTimingSummary(timings);
        return finish(1);
      }

      if (has_previous_finish_time && !timings.empty()) {
        timings.back().finish_to_next_dispatch_ms =
            elapsedMs(previous_finish_time, dispatch_time);
        logInfo() << "上一任务完成到当前指令下发间隔="
                  << timings.back().finish_to_next_dispatch_ms << " ms\n";
      }

      quad::NavigationResult final_result;
      Clock::time_point finish_time;
      if (!waitForNavigationResult(
              robot, result_waiter, &final_result, &finish_time)) {
        navigation_state_subscription.unsubscribe();
        printNavigationResult("导航最终结果", final_result);
        printTimingSummary(timings);
        return finish(1);
      }
      navigation_state_subscription.unsubscribe();

      const quad::NavigationState final_state_snapshot =
          robot.nav().navigation().getNavigationState();
      printNavigationResult("导航最终结果", final_result);
      printNavigationState("导航最终状态快照", final_state_snapshot);

      WaypointTiming timing;
      timing.name = waypoint.name;
      timing.dispatch_to_finish_ms = elapsedMs(dispatch_time, finish_time);
      timing.final_result = final_result;
      timing.final_state_snapshot = final_state_snapshot;
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
