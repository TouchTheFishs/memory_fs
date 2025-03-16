#!/bin/bash
echo "开始 memory_fs 测试..."

echo "1. 编译项目"
bash scripts/build.sh

echo "2. 准备测试环境"
mkdir -p target_dir
echo "测试文件内容" > target_dir/existing_file.txt

echo "3. 挂载文件系统"
bash scripts/mount.sh

echo "4. 编译测试程序"
cd ../build
g++ -std=c++17 ../test/test_fs_operations.cpp -o test_fs_operations

echo "5. 运行测试"
./test_fs_operations
TEST_RESULT=$?

echo "6. 卸载文件系统"
cd ../test
bash scripts/unmount.sh

echo "7. 清理环境"
rm -rf target_dir

echo "测试完成，结果: $TEST_RESULT"
exit $TEST_RESULT