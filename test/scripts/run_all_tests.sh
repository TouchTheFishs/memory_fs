#!/bin/bash
# run_all_tests.sh - 按顺序运行所有测试子脚本

# 获取当前脚本所在目录
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")

echo "===== 开始依次运行所有测试 ====="

# 1. 运行功能测试
echo "----- 运行功能测试 -----"
bash "${SCRIPT_DIR}/test_fs_operations.sh"
FS_RESULT=$?
if [ ${FS_RESULT} -ne 0 ]; then
    echo "功能测试失败，退出测试流程"
    exit ${FS_RESULT}
fi

# 2. 运行性能测试
echo "----- 运行性能测试 -----"
bash "${SCRIPT_DIR}/test_performance.sh"
PERF_RESULT=$?
if [ ${PERF_RESULT} -ne 0 ]; then
    echo "性能测试失败，退出测试流程"
    exit ${PERF_RESULT}
fi

# 3. 运行压力测试
echo "----- 运行压力测试 -----"
bash "${SCRIPT_DIR}/test_stress.sh"
STRESS_RESULT=$?
if [ ${STRESS_RESULT} -ne 0 ]; then
    echo "压力测试失败，退出测试流程"
    exit ${STRESS_RESULT}
fi

echo "===== 所有测试均已通过 ====="
exit 0
