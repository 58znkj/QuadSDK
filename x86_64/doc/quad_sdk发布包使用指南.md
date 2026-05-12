# Quad SDK 发布包使用指南

## 1. 适用范围

本文面向 Quad SDK 发布包用户，说明如何：

- 查看发布包目录
- 运行自带 `quad_cli`
- 在自己的 CMake 工程中接入 SDK

本文以下路径默认基于发布包解压后的目录结构：

```text
x86_64/
├── bin/
├── doc/
├── examples/
├── include/
└── lib/
```

## 1.1 环境要求

- Linux x86_64 发布包已在 `Ubuntu 20.04 + CMake 3.16.3 + gcc/g++ 9.4.0` 环境验证通过
- SDK 示例和最小接入工程要求 `CMake >= 3.16`、`C++17` 编译器
- `Ninja` 不是必需项；未安装时可直接使用 CMake 默认生成器（如 `Unix Makefiles`）
- 在常规 Linux 发行版上，基础 C/C++ 运行时通常由系统默认提供，无需额外处理
- 若在极简容器或裁剪系统中运行，请确认系统提供 `libcurl.so.4`
- 对较老 Linux 发行版，还需确认 `libstdc++.so.6` 提供 `GLIBCXX_3.4.26` 或更高版本符号

若是全新 Ubuntu 20.04 环境，构建客户示例工程通常至少需要先安装：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
```

## 2. 发布包目录说明

解压后常用目录如下：

- `x86_64/bin/quad_cli`
  CLI 调试工具
- `x86_64/bin/quad_example_point_nav_workflow`
  定点导航工作流示例可执行文件
- `x86_64/bin/quad_example_logging`
  日志配置示例可执行文件
- `x86_64/include/`
  头文件目录
- `x86_64/lib/`
  动态库目录
- `x86_64/lib/cmake/robot/`
  CMake 包配置目录
- `x86_64/doc/`
  随包文档目录，包含发布包使用指南、CLI 说明和版本变更说明
- `x86_64/doc/Quad_SDK版本变更说明.md`
  当前版本的对外变更记录
- `x86_64/examples/`
  最小示例工程

如果只想先验证 SDK 是否可用，可以先运行：

```bash
cd x86_64
./bin/quad_cli
```

## 3. CMake 接入的核心结论

Quad SDK 安装包已经导出了标准 CMake Config 包，使用时关键步骤只有两步：

```cmake
find_package(robot CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE robot::sdk::quad)
```

前提是让 CMake 能找到发布包中的：

```text
x86_64/lib/cmake/robot/robot-config.cmake
```

因此，`CMAKE_PREFIX_PATH` 应指向解压后的 `x86_64` 目录。

## 4. 添加 CMAKE_PREFIX_PATH 的几种方法

### 方法 1：命令行传入

最直接，推荐优先使用：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/x86_64
cmake --build build -j
```

### 方法 2：环境变量

适合一个终端会话内反复构建：

```bash
export CMAKE_PREFIX_PATH=/absolute/path/to/x86_64:$CMAKE_PREFIX_PATH
cmake -S . -B build
cmake --build build -j
```

### 方法 3：在 CMakeLists.txt 中追加

适合你明确知道 SDK 在固定目录，但不适合做通用开源工程默认写法：

```cmake
list(PREPEND CMAKE_PREFIX_PATH "/absolute/path/to/x86_64")
find_package(robot CONFIG REQUIRED)
```

### 方法 4：在 CMakePresets.json 中配置

适合团队内固定开发环境，但该方法要求 `CMake 3.21+`：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "linux-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_PREFIX_PATH": "/absolute/path/to/x86_64"
      }
    }
  ]
}
```

### 方法 5：直接指定 `robot_DIR`

如果你不想配 `CMAKE_PREFIX_PATH`，也可以直接指向包配置目录：

```bash
cmake -S . -B build \
  -Drobot_DIR=/absolute/path/to/x86_64/lib/cmake/robot
```

## 5. 最小 CMake 工程示例

### 5.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(quad_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(robot CONFIG REQUIRED)

add_executable(quad_demo
  main.cpp
)

target_link_libraries(quad_demo PRIVATE robot::sdk::quad)
```

### 5.2 main.cpp

连接地址说明：

- `192.168.3.20`：适用于开发机通过 Wi-Fi 直接连接本体。
- `192.168.1.103`：适用于在上装内进行开发、由上装侧连接本体。

下面示例默认按“上装内开发连接本体”场景填写 `192.168.1.103`。

```cpp
#include <robot/quad/api.hpp>
#include <chrono>

int main() {
  robot::quad::RobotOptions options;
  options.robot_ip = "192.168.1.103";

  robot::quad::Robot robot(options);
  if (!robot.connect(std::chrono::seconds(10))) {
    return 1;
  }
  robot.disconnect();
  return 0;
}
```

### 5.3 构建

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/x86_64
cmake --build build -j
```

## 6. 更完整的接入方式

Quad 的典型业务流程是：

1. 创建 `robot::quad::Robot`
2. `connect(timeout)`
3. `nav().resource().refreshMapResources()`
4. `nav().mapping().startLocalization()`
5. `nav().navigation().setMotionMode(robot::quad::MotionMode::Navigation)`
6. `nav().navigation().startNavigation(scene_name, path_id, ...)`

建议直接参考发布包中的示例：

- `x86_64/bin/quad_example_point_nav_workflow`
- `x86_64/bin/quad_example_logging`
- `x86_64/examples/pose_nav_workflow/main.cpp`
- `x86_64/examples/logging/main.cpp`

## 7. 使用发布包示例工程

发布包中自带了可直接构建的 examples：

```bash
cd x86_64/examples
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$(cd .. && pwd)
cmake --build build -j
```

如果你偏好 `Ninja`，也可以在 `cmake` 命令中手动补充 `-G Ninja`；未安装 `Ninja` 不影响使用发布包。

这里的 `CMAKE_PREFIX_PATH` 指向 examples 的上一级，也就是 `x86_64/`。

如果只想直接运行已编译好的示例，也可以：

```bash
cd x86_64
./bin/quad_example_point_nav_workflow
./bin/quad_example_logging
```

## 8. Linux 运行时注意事项

Quad SDK 当前以动态库形式发布，Linux 下运行你自己的程序时，需要让程序能找到：

```text
x86_64/lib/libquad.so
```

常见做法有两种。

在常规 Linux 发行版上，基础 C/C++ 运行时通常已由系统默认提供。

若在极简容器或裁剪系统中运行，请额外确认系统提供 `libcurl.so.4`；若在较老发行版上运行，还需确认 `libstdc++.so.6` 兼容 `GLIBCXX_3.4.26` 或更高版本。

### 做法 1：设置 `LD_LIBRARY_PATH`

```bash
export LD_LIBRARY_PATH=/absolute/path/to/x86_64/lib:$LD_LIBRARY_PATH
./build/quad_demo
```

### 做法 2：给可执行文件设置 RPATH

如果你的程序和 SDK 库一起打包，推荐在 CMake 中设置：

```cmake
if(UNIX AND NOT APPLE)
  set_target_properties(quad_demo PROPERTIES
    BUILD_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH "$ORIGIN/../lib")
endif()
```

这适用于可执行文件放在 `bin/`，动态库放在 `lib/` 的常见目录布局。

### 8.1 日志文件默认位置

SDK 的**默认日志文件**为**进程当前工作目录**（CWD）下的：

```text
./logs/quad_sdk.log
```

即在 CWD 下创建 `logs` 目录（若不存在）并写入 `quad_sdk.log`；从不同目录启动同一可执行文件时，日志可能落在不同绝对路径。

通过 `robot::quad::Robot::setLogFilePath()` 传入**空字符串**时，也使用上述 `./logs/quad_sdk.log`（此情况下不再读取下文中的环境变量）。

若**从未调用** `setLogFilePath`，可设置环境变量 `ROBOT_QUAD_LOG_FILE` 为非空字符串以覆盖默认路径；未设置或为空时仍使用 `./logs/quad_sdk.log`。

## 9. 常见问题

### 9.1 `find_package(robot CONFIG REQUIRED)` 失败

优先检查：

- `CMAKE_PREFIX_PATH` 是否指向了解压后的 `x86_64/`
- 或 `robot_DIR` 是否指向了 `x86_64/lib/cmake/robot`
- 发布包中的 `lib/cmake/robot/robot-config.cmake` 是否存在

### 9.2 链接成功但运行时报找不到 `.so`

这通常是运行时动态库搜索路径没有配置好。  
优先检查：

- `LD_LIBRARY_PATH`
- 可执行文件的 `RPATH`

若缺少的是 `libcurl.so.4`，请按发行版安装对应的 `libcurl` 运行时包。

若报错类似 `GLIBCXX_3.4.26 not found`，通常说明宿主机 `libstdc++` 版本过旧，建议升级运行时或使用已验证发行版环境。

### 9.3 头文件应该包含哪个入口

推荐直接包含：

```cpp
#include <robot/quad/api.hpp>
```

它会聚合 Quad SDK 常用公开头文件。

## 10. 推荐写法

对普通业务工程，推荐用下面这套最稳妥的方式：

1. 发布包解压到固定目录
2. 配置 `CMAKE_PREFIX_PATH=/absolute/path/to/x86_64`
3. `find_package(robot CONFIG REQUIRED)`
4. `target_link_libraries(your_app PRIVATE robot::sdk::quad)`
5. 运行时配置 `LD_LIBRARY_PATH` 或 `RPATH`
