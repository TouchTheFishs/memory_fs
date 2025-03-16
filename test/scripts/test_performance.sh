#!/bin/bash
# test_performance.sh - memory_fs 性能测试

SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo "===== memory_fs 性能测试 ====="

# 1. 创建测试目录
echo "1. 创建测试目录"
mkdir -p "${SCRIPT_DIR}/../mount_point"
mkdir -p "${SCRIPT_DIR}/../target_dir"
mkdir -p "${SCRIPT_DIR}/../native_dir"

# 2. 编译项目
echo "2. 编译项目"
cd "${SCRIPT_DIR}/../.."
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake ..
make -j$(nproc)

# 3. 挂载文件系统
echo "3. 挂载文件系统"
bash "${SCRIPT_DIR}/mount.sh"

# 4. 运行性能测试
echo "4. 运行性能测试"
"${SCRIPT_DIR}/../../build/test_path_utils/test_performance"
PERF_TEST_RESULT=$?

# 5. 卸载文件系统
echo "5. 卸载文件系统"
bash "${SCRIPT_DIR}/unmount.sh"

# 6. 清理环境
echo "6. 清理环境"
rm -rf "${SCRIPT_DIR}/../mount_point"
rm -rf "${SCRIPT_DIR}/../target_dir"
rm -rf "${SCRIPT_DIR}/../native_dir"

# 7. 输出测试结果
echo "===== 测试结果 ====="
if [ ${PERF_TEST_RESULT} -ne 0 ]; then
    echo "性能测试: 失败"
else
    echo "性能测试: 通过"
fi

exit ${PERF_TEST_RESULT}
