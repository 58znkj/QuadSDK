# Q10 导航统型板 BSP 二次开发接口库（libbsp）

本库将 Q10 导航统型板的板级硬件能力封装为一组稳定的 C API，覆盖**数字输入（DI）**、**数字输出（DO）**、**可控电源**、**串口设备节点**、**对频配对**与**版本查询**六类接口，供上层应用二次开发使用。

接口以动态库 `libbsp.so` 形式提供，应用只需包含头文件 `bsp.h` 并链接该库即可，无需关心 `/sys/class/gpio` 等底层实现细节。

- 当前库版本：**V1.0.0**（运行时可经 `bsp_version_get()` 读取）
- 对外头文件：`bsp.h`（同时兼容 C 与 C++，带 `extern "C"` 保护）
- 运行平台：Linux（aarch64）

---

## 目录

- [1. 特性](#1-特性)
- [2. 目录结构](#2-目录结构)
- [3. 编译与链接](#3-编译与链接)
- [4. 快速开始](#4-快速开始)
- [5. API 参考](#5-api-参考)
- [6. 使用说明](#6-使用说明)
- [7. 版本历史](#8-版本历史)

---

## 1. 特性

| 能力 | 说明 |
|---|---|
| 数字输入 DI | 2 路：1 路通用 DI + 1 路急停专用 E-STOP |
| 数字输出 DO | 4 路，可写可回读 |
| 可控电源 | 11 路独立使能（24V / 12V / 5V / 隔离电源 / USB 供电 / 对频模块供电），支持单路与全开全关 |
| 串口 | 按通道号返回设备节点（`如：/dev/ttyS0`） |
| 对频配对 | 一键使本机对频模块进入配对模式 |
| 版本号 | 运行时读取库版本字符串 |

---

## 2. 目录结构

| 文件 | 说明 |
|---|---|
| `bsp.h` | 对外唯一头文件，包含全部错误码、通道枚举与函数原型 |
| `libbsp.so` | bsp动态库，上层应用只需包含对应头文件 bsp.h 并链接该动态库，即可完成接口的调用 |

---

## 3. 链接

### 3.1 应用链接

需显式指定头文件路径、库路径与运行时搜索路径：

```bash
gcc -o your_app main.c -I./include -L./lib -lbsp
```

•	-I./include：指定 bsp.h 所在的头文件目录
•	-L./lib：指定 libbsp.so 所在的库文件目录
•	-lbsp：链接 libbsp.so 动态库


---

## 4. 快速开始

```c
#include <stdio.h>
#include "bsp.h"

int main(void)
{
    int level;

    printf("libbsp version: %s\n", bsp_version_get());

    /* 读取急停输入 */
    if (bsp_di_read(BSP_DI_EMERGENCY, &level) == BSP_OK)
        printf("E-STOP = %d\n", level);

    /* 串口设备节点 */
    printf("UART0 node: %s\n", bsp_uart_dev_node_get(BSP_UART_0));

    /* 打开 5V 电源，再关闭 */
    bsp_power_set(BSP_PWR_5V, 1);
    bsp_power_get(BSP_PWR_5V, &level);
    printf("PWR_5V = %d\n", level);
    bsp_power_set(BSP_PWR_5V, 0);

    /* 拉高 DO0 */
    bsp_do_write(BSP_DO_CH0, 1);
    bsp_do_read(BSP_DO_CH0, &level);
    printf("DO0 = %d\n", level);

    return 0;
}
```

---

## 5. API 参考

### 5.1 错误码

| 宏 | 值 | 含义 |
|---|---|---|
| `BSP_OK` | `0` | 操作成功 |
| `BSP_ERR_PARAM` | `-1` | 参数错误（通道越界、`level`/`on` 非 0/1、输出指针为 `NULL`） |
| `BSP_ERR_OPEN` | `-2` | 设备（sysfs 节点）打开失败 |
| `BSP_ERR_WRITE` | `-3` | 写入失败 |
| `BSP_ERR_READ` | `-4` | 读取失败 |

> 实现中多数对外接口会把内部错误归一化：例如 `bsp_di_read` 失败统一返回 `BSP_ERR_READ`，`bsp_do_write`/`bsp_power_set` 失败统一返回 `BSP_ERR_WRITE`。

### 5.2 通道定义

**数字输入 DI**（`bsp_di_ch_t`，共 `BSP_DI_COUNT = 2` 路）

| 枚举 | 值 | 说明 |
|---|---|---|
| `BSP_DI_CH0` | 0 | 通用数字输入通道 DI1 |
| `BSP_DI_EMERGENCY` | 1 | 急停专用数字输入通道 E-STOP |

**数字输出 DO**（`bsp_do_ch_t`，共 `BSP_DO_COUNT = 4` 路）

| 枚举 | 值 | 说明 |
|---|---|---|
| `BSP_DO_CH0` | 0 | 通用数字输出通道 DO1 |
| `BSP_DO_CH1` | 1 | 通用数字输出通道 DO2 |
| `BSP_DO_CH2` | 2 | 通用数字输出通道 DO3 |
| `BSP_DO_CH3` | 3 | 通用数字输出通道 DO4 |

**可控电源**（`bsp_power_ch_t`，共 `BSP_PWR_COUNT = 11` 路）

输出高电平（1）= 打开电源，输出低电平（0）= 关闭电源。

| 枚举 | 值 | 说明 |
|---|---|---|
| `BSP_PWR1_24V` | 0 | 第 1 路 24V |
| `BSP_PWR_ISO_12V` | 1 | 隔离电源 12V |
| `BSP_PWR_5V` | 2 | 5V 电源 |
| `BSP_PWR1_12V` | 3 | 第 1 路 12V |
| `BSP_PWR2_24V` | 4 | 第 2 路 24V |
| `BSP_PWR2_12V` | 5 | 第 2 路 12V |
| `BSP_PWR_USB1` | 6 | USB1 电源 5V |
| `BSP_PWR_USB2` | 7 | USB2 电源 5V |
| `BSP_PWR_USB3` | 8 | USB3 电源 5V |
| `BSP_PWR_USB4` | 9 | USB4 电源 5V |
| `BSP_PWR_FREQ` | 10 | 本体内对频模块电源 12V |

**串口**（`bsp_uart_ch_t`，共 `BSP_UART_COUNT = 2` 路）

| 枚举 | 值 | 设备节点 |
|---|---|---|
| `BSP_UART_0` | 0 |
| `BSP_UART_1` | 1 |

### 5.3 接口一览

| 接口 | 功能 | 成功返回 |
|---|---|---|
| `int bsp_di_read(bsp_di_ch_t ch, int *level)` | 读取一路 DI 电平 | `BSP_OK` |
| `int bsp_do_write(bsp_do_ch_t ch, int level)` | 设置一路 DO 输出电平 | `BSP_OK` |
| `int bsp_do_read(bsp_do_ch_t ch, int *level)` | 回读一路 DO 当前电平 | `BSP_OK` |
| `int bsp_power_set(bsp_power_ch_t ch, int on)` | 打开/关闭一路电源 | `BSP_OK` |
| `int bsp_power_get(bsp_power_ch_t ch, int *on)` | 读取一路电源开关状态 | `BSP_OK` |
| `int bsp_power_set_all(int on)` | 全开/全关所有电源 | `BSP_OK` |
| `const char *bsp_uart_dev_node_get(bsp_uart_ch_t ch)` | 获取串口设备节点 | 节点字符串（失败 `NULL`） |
| `int bsp_freq_match(void)` | 触发对频配对 | `BSP_OK` |
| `const char *bsp_version_get(void)` | 获取库版本号 | 版本字符串 |

### 5.4 DI 接口

#### `int bsp_di_read(bsp_di_ch_t ch, int *level)`

读取一路数字输入的电平。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | DI 通道号，`BSP_DI_CH0` 或 `BSP_DI_EMERGENCY` |
| `level` | 出 | 返回电平：`1` 高电平，`0` 低电平 |

- 返回：`BSP_OK` 成功；通道越界或 `level` 为空返回 `BSP_ERR_PARAM`；读取失败返回 `BSP_ERR_READ`。

```c
int estop;
if (bsp_di_read(BSP_DI_EMERGENCY, &estop) == BSP_OK)
    printf("急停输入 = %s\n", estop ? "高" : "低");
```

> `BSP_DI_EMERGENCY` 为急停专用 DI，上层判断急停状态时建议配合软件滤波/去抖。

### 5.5 DO 接口

#### `int bsp_do_write(bsp_do_ch_t ch, int level)`

设置一路数字输出的电平。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | DO 通道号，`BSP_DO_CH0` ~ `BSP_DO_CH3` |
| `level` | 入 | 输出电平：`1` 高电平，`0` 低电平（其他值非法） |

- 返回：`BSP_OK` 成功；参数非法返回 `BSP_ERR_PARAM`；写入失败返回 `BSP_ERR_WRITE`。

#### `int bsp_do_read(bsp_do_ch_t ch, int *level)`

回读一路数字输出的当前电平。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | DO 通道号 |
| `level` | 出 | 当前输出电平 |

- 返回：`BSP_OK` 成功；参数非法返回 `BSP_ERR_PARAM`；读取失败返回 `BSP_ERR_READ`。

```c
bsp_do_write(BSP_DO_CH2, 1);          /* 拉高 */
int v;
bsp_do_read(BSP_DO_CH2, &v);          /* 回读校验 */
```

### 5.6 电源控制接口

#### `int bsp_power_set(bsp_power_ch_t ch, int on)`

打开或关闭一路可控电源，语义与 DO 相同：输出高电平（1）= 打开，低电平（0）= 关闭。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | 电源通道号，`BSP_PWR1_24V` ~ `BSP_PWR_FREQ` |
| `on` | 入 | `1` 打开，`0` 关闭（其他值非法） |

- 返回：`BSP_OK` 成功；参数非法返回 `BSP_ERR_PARAM`；写入失败返回 `BSP_ERR_WRITE`。

```c
bsp_power_set(BSP_PWR_USB1, 1);   /* 给 USB1 供电 */
bsp_power_set(BSP_PWR_USB1, 0);   /* 断电 */
```

#### `int bsp_power_get(bsp_power_ch_t ch, int *on)`

读取一路电源的当前开关状态。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | 电源通道号 |
| `on` | 出 | `1` 开，`0` 关 |

- 返回：`BSP_OK` 成功；参数非法返回 `BSP_ERR_PARAM`；读取失败返回 `BSP_ERR_READ`。

#### `int bsp_power_set_all(int on)`

同时打开或关闭全部 11 路电源，内部按通道顺序 `0 → 10` 依次调用 `bsp_power_set()`，**遇到错误立即返回**。

| 参数 | 方向 | 说明 |
|---|---|---|
| `on` | 入 | `1` 全部打开，`0` 全部关闭 |

- 返回：`BSP_OK` 成功；`on` 非法返回 `BSP_ERR_PARAM`；任一通道失败返回 `BSP_ERR_WRITE`。

```c
bsp_power_set_all(1);   /* 全上电 */
/* ... 业务 ... */
bsp_power_set_all(0);   /* 全断电 */
```

> 注意：全关会同时切断对频模块、USB 等供电，可能导致正在运行的外设掉线，请按业务需要选择单路控制。

### 5.7 串口设备节点接口

#### `const char *bsp_uart_dev_node_get(bsp_uart_ch_t ch)`

按通道号返回串口设备节点，例如 `"/dev/ttyS0"`。库只负责返回节点，串口的打开、配置与读写由应用自行完成（如 `open()` + `termios`）。

| 参数 | 方向 | 说明 |
|---|---|---|
| `ch` | 入 | 串口通道号，`BSP_UART_0` / `BSP_UART_1` |

- 返回：指向内部静态字符串的常量指针；

```c
const char *dev = bsp_uart_dev_node_get(BSP_UART_1);
int fd = open(dev, O_RDWR | O_NOCTTY);
```

### 5.8 对频配对接口

#### `int bsp_freq_match(void)`

调用后本机对频模块进入对频配对模式。**终端侧的对频模块也需同时进入配对模式**，双方才能完成最终配对。

- 该接口为**阻塞调用**，较为耗时，请勿在实时性要求高的线程中调用。
- 返回：`BSP_OK` 成功；电源操作失败返回 `BSP_ERR_WRITE`。

```c
printf("开始对频，请在终端侧同步触发配对...\n");
if (bsp_freq_match() == BSP_OK)
    printf("对频流程完成\n");
```

### 5.9 版本号接口

#### `const char *bsp_version_get(void)`

返回库版本字符串，如 `"1.0.0"`（不含前缀 `V`）。

- 返回：指向静态字符串的常量指针，无需释放。

```c
printf("libbsp version = V%s\n", bsp_version_get());
```

---

## 6. 使用说明

**权限**：执行编译链接后生成的可执行文件your_app时，需以超级用户身份执行，否则调用bsp接口会因缺失权限导致接口调用不生效。例如：sudo ./your_app


**电平语义**：DO 与电源均为 `1` = 高电平，电源通道中高电平即“打开电源”。

**错误处理**：所有接口返回 `BSP_OK` 才表示成功，建议对返回值做判断：

```c
int ret = bsp_power_set(BSP_PWR2_24V, 1);
if (ret != BSP_OK)
    fprintf(stderr, "power on failed: %d\n", ret);
```

**头文件**： `#include "bsp.h"`


---

## 7. 版本历史
| 版本 | 日期 | 说明 |
|---|---|---|
| 1.0.0 | 2026-11-04 | 首版：DI / DO / 电源 / 串口节点 / 对频 / 版本号 |
