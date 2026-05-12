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
  - 展示连续定点导航队列：上一个导航点执行到终态后，再下发下一个导航点，并统计“指令下发到任务完成”和“任务完成到下一个指令下发”的耗时，单位为 ms。
  - 运行参数：`quad_example_multi_point_nav_queue [robot_ip] [scene_name]`；导航点列表在源码 `waypoints` 中配置。
  - 发布包中对应二进制：`bin/quad_example_multi_point_nav_queue`

- `logging/`
  - target: `quad_example_logging`
  - 展示日志配置：设置日志等级、文件路径和日志回调。
  - 发布包中对应二进制：`bin/quad_example_logging`

## 运行前修改

运行前请先修改各示例目录中的源码配置，替换机器人 IP、场景名、路径 ID 和日志文件路径等配置。
