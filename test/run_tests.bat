@echo off
echo 开始 memory_fs 测试...

echo 1. 编译项目
call scripts\build.bat

echo 2. 准备测试环境
mkdir -p target_dir
echo 测试文件内容 > target_dir\existing_file.txt

echo 3. 挂载文件系统
call scripts\mount.bat

echo 4. 编译测试程序
cd ..\build
g++ -std=c++17 ..\test\test_fs_operations.cpp -o test_fs_operations.exe

echo 5. 运行测试
.\test_fs_operations.exe
set TEST_RESULT=%ERRORLEVEL%

echo 6. 卸载文件系统
cd ..\test
call scripts\unmount.bat

echo 测试完成，结果代码: %TEST_RESULT%
exit /b %TEST_RESULT%