# Quad SDK 示例

## 环境要求

- CMake >= 3.16
- C++17 编译器
- Linux x86_64 发布包已在 `Ubuntu 20.04 + CMake 3.16.3 + gcc/g++ 9.4.0` 环境验证通过
- 在常规 Linux 发行版上，基础 C/C++ 运行时通常由系统默认提供，无需额外处理
- 若在极简容器或裁剪系统中运行，请确认系统提供 `libcurl.so.4`，并确保 `libstdc++.so.6` 兼容 `GLIBCXX_3.4.26` 或更高版本
- `Ninja` 不是必需项；若未安装，可直接使用 CMake 默认生成器（如 `Unix Makefiles`）

## 构建

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/SDK_Quad_Linux_x86_64/x86_64
cmake --build build -j4
```

如需显式使用 `Ninja`，可在上面的 `cmake` 命令中额外添加 `-G Ninja`。

如果 Quad SDK 在主工程中以 `ROBOT_BUILD_QUAD_EXAMPLES=ON` 构建，
这些示例也会作为主工程 target 一起编译，并在安装/打包时发布到 `bin/`。

## 示例说明

- `pose_nav_workflow/`
  - target: `quad_example_point_nav_workflow`
  - 展示定点导航工作流：连接、刷新地图资源、带初始位姿的重定位、设置运动模式、`startNavigation(scene, pose, ...)`、停障查询、暂停/恢复与取消导航，并监听导航结果。
  - 发布包中对应二进制：`bin/quad_example_point_nav_workflow`

- `multi_point_nav_queue/`
  - target: `quad_example_multi_point_nav_queue`
  - 展示连续定点导航队列：支持等待上一个导航点到终态后再下发下一个导航点，也支持 `--advance-after-ms` 提前下发模式；统计“指令下发到任务完成/提前切换”和“完成/提前切换到下一个指令下发”的耗时，单位为 ms。
  - 运行参数：`quad_example_multi_point_nav_queue --robot-ip 192.168.3.20 --scene 430 --point point_1,1,0,0,1,0.01,0.001,-0.006 --point point_2,2,0,0,1,0.01,0.001,-0.004`。
  - 提前下发示例：追加 `--advance-after-ms 3000`，当前点执行 3000 ms 后若还未到终态，直接下发下一个点。
  - 当前调试配置脚本：`multi_point_nav_queue/run_current_config.sh`，安装后位于 `bin/quad_example_multi_point_nav_queue_current_config.sh`；可直接修改脚本中的默认机器人 IP、场景名和导航点，也可通过 `ROBOT_IP`、`SCENE_NAME`、`ADVANCE_AFTER_MS`、`FINISH_TIMEOUT_MS` 覆盖默认值。
  - 发布包中对应二进制：`bin/quad_example_multi_point_nav_queue`

- `logging/`
  - target: `quad_example_logging`
  - 展示日志配置：设置日志等级、文件路径和日志回调。
  - 发布包中对应二进制：`bin/quad_example_logging`

## 运行前修改

运行前请根据示例参数或脚本环境变量替换机器人 IP、场景名、路径 ID、导航点和日志文件路径等配置。
