# Quad SDK

Quad SDK 是面向四足机器人导航业务的二进制发布包。本仓库只发布已编译 SDK、示例、命令行工具和使用文档，不包含 SDK 内部源码。

当前版本：`0.1.3`

## 选择合适的发布包

请根据运行 SDK 的目标设备架构选择目录或 Release 附件：

| 目标环境 | 选择 |
| --- | --- |
| Linux x86_64 / amd64，例如普通 PC、工控机、x86_64 Ubuntu | `x86_64/` 或 `SDK_Quad_Linux_x86_64.zip` |
| Linux arm64 / aarch64，例如 ARM 上装、ARM 开发板 | `arm64/` 或 `SDK_Quad_Linux_arm64.zip` |

如果不确定当前机器架构，可在目标设备上执行：

```bash
uname -m
```

常见输出对应关系：

- `x86_64`：使用 `x86_64` 包
- `aarch64` 或 `arm64`：使用 `arm64` 包

## 发布包内容

每个平台目录结构基本一致：

```text
<arch>/
├── bin/       # quad_cli 和示例可执行文件
├── doc/       # 使用指南、CLI 说明、接口规格书、版本变更说明
├── examples/  # 可直接编译的 CMake 示例工程
├── include/   # C++ 公开头文件
└── lib/       # libquad.so 和 CMake Config 包
```

其中 `<arch>` 为 `x86_64` 或 `arm64`。

## 先阅读哪些文档

建议按下面顺序阅读：

1. `x86_64/doc/quad_sdk发布包使用指南.md` 或 `arm64/doc/quad_sdk发布包使用指南.md`
   说明环境要求、目录结构、CMake 接入方式和最小工程示例。
2. `x86_64/doc/四足机器够导航SDK接口规格书.md` 或 `arm64/doc/四足机器够导航SDK接口规格书.md`
   说明 SDK 对外接口、数据结构、导航流程和错误码。
3. `x86_64/doc/quad_cli使用说明.md` 或 `arm64/doc/quad_cli使用说明.md`
   说明如何使用随包提供的 `quad_cli` 做联调和排障。
4. `x86_64/doc/Quad_SDK版本变更说明.md` 或 `arm64/doc/Quad_SDK版本变更说明.md`
   查看当前版本功能变更和兼容性说明。

如果你只是想先把 SDK 接入自己的 CMake 工程，优先阅读 `quad_sdk发布包使用指南.md`。

## 安装基础环境

SDK 示例和客户工程要求：

- Linux `x86_64` 或 `arm64`
- 支持 C++17 的编译器
- `CMake >= 3.16`
- 系统可用的基础 C/C++ 运行时
- 极简系统或容器中需确认存在 `libcurl.so.4`
- 较老 Linux 发行版需确认 `libstdc++.so.6` 提供 `GLIBCXX_3.4.26` 或更高版本符号

全新 Ubuntu 20.04/22.04 环境通常可先安装：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
```

如果希望使用 Ninja 构建，也可以额外安装：

```bash
sudo apt-get install -y ninja-build
```

## 快速验证 SDK

进入对应架构目录后，可以先运行 CLI：

```bash
cd x86_64
./bin/quad_cli
```

或在 ARM64 设备上：

```bash
cd arm64
./bin/quad_cli
```

如果程序提示缺少动态库，请先确认当前系统架构选择正确，并检查系统运行时依赖。

## CMake 接入方式

客户工程中推荐使用标准 CMake Config 包：

```cmake
find_package(robot CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE robot::sdk::quad)
```

配置工程时，让 `CMAKE_PREFIX_PATH` 指向解压后的平台目录。例如使用 x86_64 包：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/SDK_Quad/x86_64
cmake --build build -j
```

使用 arm64 包时，将路径替换为：

```bash
-DCMAKE_PREFIX_PATH=/absolute/path/to/SDK_Quad/arm64
```

更完整的最小工程示例请参考：

- `x86_64/examples/`
- `arm64/examples/`
- 对应架构目录下的 `doc/quad_sdk发布包使用指南.md`

## 机器人连接地址提示

示例文档中常见连接地址：

- `192.168.3.20`：开发机通过 Wi-Fi 直接连接机器人本体时常用
- `192.168.1.103`：在上装内开发，由上装侧连接机器人本体时常用

实际项目中请按现场网络配置修改 `robot::quad::RobotOptions::robot_ip`。

## 版本与变更

SDK 内部版本可通过头文件宏或函数读取：

- `ROBOT_QUAD_VERSION_STRING`
- `robot::quad::sdk_version_string()`

每次升级前建议先阅读对应平台目录下的：

```text
doc/Quad_SDK版本变更说明.md
```

## 常见问题

### 应该下载目录还是 zip 包？

如果你从 GitHub Release 下载，优先选择对应平台的 zip 包。解压后目录结构与本仓库中的 `x86_64/`、`arm64/` 一致。

### 可以混用不同平台的包吗？

不可以。`x86_64` 包只能在 x86_64 Linux 上使用，`arm64` 包只能在 arm64/aarch64 Linux 上使用。

### SDK 是否包含源码？

不包含内部源码。发布包只包含公开头文件、动态库、示例源码、命令行工具和文档。

### 示例源码能否参考？

可以。`examples/` 中的源码用于展示推荐接入方式，适合作为客户工程的起点。
