#include <robot/quad/api.hpp>

#include <chrono>
#include <iostream>
#include <string>

namespace {

constexpr auto kRobotConnectTimeout = std::chrono::seconds(10);

const char* logLevelText(robot::quad::LogLevel level) {
  switch (level) {
    case robot::quad::LogLevel::DEBUG:
      return "调试";
    case robot::quad::LogLevel::INFO:
      return "信息";
    case robot::quad::LogLevel::WARN:
      return "警告";
    case robot::quad::LogLevel::ERROR:
      return "错误";
  }
  return "未知";
}

}  // namespace

int main() {
  try {
    const std::string robot_ip = "192.168.1.103";
    const std::string log_file = "./logs/quad_example.log";

    // 在构造 Robot 前完成日志配置。
    robot::quad::Robot::setLogLevel(robot::quad::LogLevel::INFO);
    robot::quad::Robot::setConsoleLogLevel(robot::quad::LogLevel::INFO);
    robot::quad::Robot::setFileLogLevel(robot::quad::LogLevel::DEBUG);
    robot::quad::Robot::setLogFilePath(log_file);
    robot::quad::Robot::setLogCallback(
        [](robot::quad::LogLevel level,
           const std::string& logger_name,
           const std::string& message) {
          std::cout << "[SDK][" << logLevelText(level) << "] "
                    << logger_name << "： " << message << "\n";
        });

    robot::quad::RobotOptions options;
    options.robot_ip = robot_ip;

    robot::quad::Robot robot(options);
    if (!robot.connect(kRobotConnectTimeout)) {
      std::cerr << "机器人连接失败：" << robot.getRobotIp() << "\n";
      return 1;
    }

    std::cout << "已连接机器人：" << robot.getRobotIp() << "\n";
    std::cout << "控制台日志等级：INFO\n";
    std::cout << "文件日志等级：DEBUG\n";
    std::cout << "日志文件路径：" << log_file << "\n";

    robot.disconnect();
    std::cout << "日志示例执行完成。\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "日志示例执行失败：" << e.what() << "\n";
    return 1;  // 日志示例执行异常
  }
}
