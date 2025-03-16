#!/bin/bash
echo "===== memory_fs 测试套件 ====="

echo "1. 创建测试目录"
mkdir -p mount_point
mkdir -p target_dir
mkdir -p native_dir

echo "2. 编译项目"
cd ..
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake ..
make -j$(nproc)
cd ../test

echo "3. 运行路径工具测试"
../build/test_path_utils
if [ $? -ne 0 ]; then
    echo "路径工具测试失败！"
    exit $?
fi

echo "4. 挂载文件系统"
bash scripts/mount.sh

echo "5. 运行功能测试"
../build/test_fs_operations
FS_TEST_RESULT=$?

echo "6. 运行性能测试"
../build/test_performance
PERF_TEST_RESULT=$?

echo "7. 运行压力测试"
../build/test_stress
STRESS_TEST_RESULT=$?

echo "8. 卸载文件系统"
bash scripts/unmount.sh

echo "9. 清理环境"
rm -rf mount_point
rm -rf target_dir
rm -rf native_dir

echo "===== 测试结果 ====="
if [ $FS_TEST_RESULT -ne 0 ]; then
    echo "功能测试: 失败"
else
    echo "功能测试: 通过"
fi

if [ $PERF_TEST_RESULT -ne 0 ]; then
    echo "性能测试: 失败"
else
    echo "性能测试: 通过"
fi

if [ $STRESS_TEST_RESULT -ne 0 ]; then
    echo "压力测试: 失败"
else
    echo "压力测试: 通过"
fi

if [ $FS_TEST_RESULT -ne 0 ]; then exit $FS_TEST_RESULT; fi
if [ $PERF_TEST_RESULT -ne 0 ]; then exit $PERF_TEST_RESULT; fi
if [ $STRESS_TEST_RESULT -ne 0 ]; then exit $STRESS_TEST_RESULT; fi