#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../../../.." && pwd)"

# 客户可按现场环境修改下方默认配置，也可以在执行脚本时通过环境变量覆盖：
#   ROBOT_IP=192.168.3.20 SCENE_NAME=430 ./quad_example_multi_point_nav_queue_current_config.sh
#   ADVANCE_AFTER_MS=2000 ./quad_example_multi_point_nav_queue_current_config.sh
#
# EXE:
#   导航示例程序路径。安装包内脚本位于 bin 目录，默认执行同目录下的
#   quad_example_multi_point_nav_queue；源码树调试运行时会回退到
#   output/build-debug-offline/quad_example_multi_point_nav_queue。
# ROBOT_IP:
#   机器人 IP 地址。
# SCENE_NAME:
#   导航使用的场景名称，需要与机器人中已有地图场景一致。
# ADVANCE_AFTER_MS:
#   提前下发模式。0 表示关闭；大于 0 时，当前点执行指定毫秒数后直接下发下一个点。
# FINISH_TIMEOUT_MS:
#   普通模式下单个导航点等待终态的超时时间，单位毫秒。
DEFAULT_EXE="${SCRIPT_DIR}/quad_example_multi_point_nav_queue"
if [[ ! -x "${DEFAULT_EXE}" ]]; then
  DEFAULT_EXE="${REPO_ROOT}/output/build-debug-offline/quad_example_multi_point_nav_queue"
fi

EXE="${EXE:-${DEFAULT_EXE}}"
ROBOT_IP="${ROBOT_IP:-192.168.3.20}"
SCENE_NAME="${SCENE_NAME:-430}"
ADVANCE_AFTER_MS="${ADVANCE_AFTER_MS:-0}"
FINISH_TIMEOUT_MS="${FINISH_TIMEOUT_MS:-10000}"

if [[ ! -x "${EXE}" ]]; then
  echo "executable not found: ${EXE}" >&2
  echo "build it with: cmake --build --preset linux-debug-offline --target quad_example_multi_point_nav_queue -j" >&2
  exit 1
fi

cmd=(
  "${EXE}"
  --robot-ip "${ROBOT_IP}"
  --scene "${SCENE_NAME}"
  --finish-timeout-ms "${FINISH_TIMEOUT_MS}"
  # 导航点格式：名称,x,y,z,qw,qx,qy,qz。按客户现场地图替换或增删 --point 行。
  --point "point_1,1,0,0,1.000,0.010,0.001,-0.006"
  --point "point_2,2,0,0,1,0.010,0.001,-0.004"
  --point "point_3,3,0,0,0.872,0.017,0.004,0.489"
)

if [[ "${ADVANCE_AFTER_MS}" != "0" ]]; then
  cmd+=(--advance-after-ms "${ADVANCE_AFTER_MS}")
fi

exec "${cmd[@]}"
