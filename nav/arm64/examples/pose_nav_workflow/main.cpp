#include <robot/quad/api.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>

/** @brief 定点导航工作流示例。演示连接、刷新资源、重定位、定点导航、停障查询、暂停恢复与取消导航。 */

int main() {
    using namespace robot::quad;

    const std::string scene_name = "scene_1";
    const Pose relocalization_pose = poseFromXYYawDegrees(0.0f, 0.0f, 0.0f);
    const Pose2D relocalization_pose_2d = pose2DFromPose(relocalization_pose);
    const Pose target_pose = poseFromXYYawDegrees(2.0f, 0.0f, 0.0f);
    const Pose2D target_pose_2d = pose2DFromPose(target_pose);

    RobotOptions options;
    options.robot_ip = "192.168.1.103";
    Robot robot(options);

    auto finish = [&robot](int ret_code) {
        if (robot.isConnected()) {
            robot.disconnect();
            std::cout << "机器人断开连接\n";
        }
        return ret_code;
    };

    // 第 1 步：创建 Robot 并连接机器人

    if (!robot.connect(std::chrono::seconds(10))) {
        std::cerr << "机器人连接失败\n";
        return finish(1);
    }
    std::cout << "机器人连接成功\n";

    // 第 2 步：刷新地图资源，并确认场景存在

    std::promise<ErrorCode> refresh_result_promise;
    auto refresh_result_future = refresh_result_promise.get_future();
    robot.nav().resource().refreshMapResources(
        [&refresh_result_promise](const AsyncCommandResult& result) {
            refresh_result_promise.set_value(result.error);
        });

    if (refresh_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "刷新地图资源失败\n";
        return finish(1);
    }
    std::cout << "刷新地图资源成功\n";

    MapResourceSnapshot map_resource = robot.nav().resource().getMapResources();
    bool scene_found = false;
    for (const SceneDetail& scene : map_resource.scenes) {
        if (scene.scene_name == scene_name) {
            scene_found = true;
            break;
        }
    }

    if (!scene_found) {
        std::cerr << "场景 " << scene_name << " 不存在\n";
        return finish(1);
    }
    std::cout << "场景 " << scene_name << " 存在\n";

    // 第 3 步：执行重定位

    // 这里演示带 initial_pose 的 startLocalization()，适合做重定位。
    std::cout << "重定位初始位姿"
              << " x=" << relocalization_pose_2d.x
              << " y=" << relocalization_pose_2d.y
              << " yaw=" << relocalization_pose_2d.yaw_degree
              << "\n";

    std::promise<ErrorCode> localization_result_promise;
    auto localization_result_future = localization_result_promise.get_future();
    robot.nav().mapping().startLocalization(
        scene_name,
        relocalization_pose,
        [&localization_result_promise](const AsyncCommandResult& result) {
            localization_result_promise.set_value(result.error);
        });

    if (localization_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "启动重定位失败\n";
        return finish(1);
    }

    LocalizationState localization_state;
    auto localization_deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (std::chrono::steady_clock::now() < localization_deadline) {
        localization_state = robot.nav().mapping().getLocalizationState();
        if (localization_state.ready && localization_state.fresh) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (!localization_state.ready || !localization_state.fresh) {
        std::cerr << "重定位后定位未就绪\n";
        return finish(1);
    }

    std::cout << "重定位完成"
              << " x=" << localization_state.pose.position.x
              << " y=" << localization_state.pose.position.y
              << "\n";

    // 第 4 步：启动定位导航

    if (robot.nav().navigation().getMotionMode() != MotionMode::Navigation) {
        std::promise<ErrorCode> motion_mode_result_promise;
        auto motion_mode_result_future = motion_mode_result_promise.get_future();
        robot.nav().navigation().setMotionMode(
            MotionMode::Navigation,
            [&motion_mode_result_promise](const AsyncCommandResult& result) {
                motion_mode_result_promise.set_value(result.error);
            });

        if (motion_mode_result_future.get() != ErrorCode::SUCCESS) {
            std::cerr << "切换导航模式失败\n";
            return finish(1);
        }
    }

    std::cout << "目标导航点"
              << " x=" << target_pose_2d.x
              << " y=" << target_pose_2d.y
              << " yaw=" << target_pose_2d.yaw_degree
              << "\n";

    std::promise<ErrorCode> start_navigation_result_promise;
    auto start_navigation_result_future = start_navigation_result_promise.get_future();
    robot.nav().navigation().startNavigation(
        scene_name,
        target_pose,
        [&start_navigation_result_promise](const AsyncCommandResult& result) {
            start_navigation_result_promise.set_value(result.error);
        });

    if (start_navigation_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "启动定点导航失败\n";
        return finish(1);
    }
    std::cout << "启动定点导航成功\n";

    // 第 5 步：读取当前导航状态，演示停障状态获取

    std::this_thread::sleep_for(std::chrono::seconds(2));
    NavigationState navigation_state = robot.nav().navigation().getNavigationState();
    std::cout << "当前导航状态"
              << " status=" << static_cast<int>(navigation_state.status)
              << " task_id=" << navigation_state.task_id
              << " error=" << static_cast<int>(navigation_state.error)
              << " output_status="
              << static_cast<uint32_t>(navigation_state.output_status)
              << "\n";

    if (navigation_state.error == ErrorCode::NAV_STOP_BARRIER ||
        navigation_state.error == ErrorCode::NAV_STOP_BARRIER_ON_STAIR) {
        std::cout << "当前处于停障状态\n";
    }

    // 第 6 步：暂停与恢复

    // 这里直接演示 setNavigationPaused() 和 getPauseState() 的调用方式。
    std::promise<ErrorCode> pause_result_promise;
    auto pause_result_future = pause_result_promise.get_future();
    robot.nav().navigation().setNavigationPaused(
        true,
        [&pause_result_promise](const AsyncCommandResult& result) {
            pause_result_promise.set_value(result.error);
        });

    if (pause_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "暂停导航失败\n";
        return finish(1);
    }

    PauseState pause_state = robot.nav().navigation().getPauseState();
    std::cout << "暂停导航成功"
              << " paused=" << pause_state.paused
              << " fresh=" << pause_state.fresh
              << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::promise<ErrorCode> resume_result_promise;
    auto resume_result_future = resume_result_promise.get_future();
    robot.nav().navigation().setNavigationPaused(
        false,
        [&resume_result_promise](const AsyncCommandResult& result) {
            resume_result_promise.set_value(result.error);
        });

    if (resume_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "恢复导航失败\n";
        return finish(1);
    }

    pause_state = robot.nav().navigation().getPauseState();
    std::cout << "恢复导航成功"
              << " paused=" << pause_state.paused
              << " fresh=" << pause_state.fresh
              << "\n";

    // 第 7 步：取消导航，并轮询最终导航状态

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::promise<ErrorCode> stop_result_promise;
    auto stop_result_future = stop_result_promise.get_future();
    robot.nav().navigation().stopNavigation(
        [&stop_result_promise](const AsyncCommandResult& result) {
            stop_result_promise.set_value(result.error);
        });

    if (stop_result_future.get() != ErrorCode::SUCCESS) {
        std::cerr << "取消导航失败\n";
        return finish(1);
    }
    std::cout << "取消导航成功\n";

    NavigationState final_navigation_state;
    auto navigation_deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < navigation_deadline) {
        final_navigation_state = robot.nav().navigation().getNavigationState();
        if (final_navigation_state.status == NavigationStatus::FINISHED ||
            final_navigation_state.status == NavigationStatus::CANCELLED ||
            final_navigation_state.status == NavigationStatus::FAILED) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "最终导航状态"
              << " status=" << static_cast<int>(final_navigation_state.status)
              << " task_id=" << final_navigation_state.task_id
              << " error=" << static_cast<int>(final_navigation_state.error)
              << " output_status="
              << static_cast<uint32_t>(final_navigation_state.output_status)
              << "\n";

    return finish(0);
}
