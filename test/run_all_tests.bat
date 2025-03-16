@echo off
echo ===== memory_fs 测试套件 =====

echo 1. 创建测试目录
mkdir mount_point 2>nul
mkdir target_dir 2>nul
mkdir native_dir 2>nul

echo 2. 编译项目
cd ..
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
cd ..\test

echo 3. 运行路径工具测试
..\build\Release\test_path_utils.exe
if %ERRORLEVEL% neq 0 (
    echo 路径工具测试失败！
    exit /b %ERRORLEVEL%
)

echo 4. 挂载文件系统
call scripts\mount.bat

echo 5. 运行功能测试
..\build\Release\test_fs_operations.exe
set FS_TEST_RESULT=%ERRORLEVEL%

echo 6. 运行性能测试
..\build\Release\test_performance.exe
set PERF_TEST_RESULT=%ERRORLEVEL%

echo 7. 运行压力测试
..\build\Release\test_stress.exe
set STRESS_TEST_RESULT=%ERRORLEVEL%

echo 8. 卸载文件系统
call scripts\unmount.bat

echo 9. 清理环境
rmdir /s /q mount_point 2>nul
rmdir /s /q target_dir 2>nul
rmdir /s /q native_dir 2>nul

echo ===== 测试结果 =====
if %FS_TEST_RESULT% neq 0 (
    echo 功能测试: 失败
) else (
    echo 功能测试: 通过
)

if %PERF_TEST_RESULT% neq 0 (
    echo 性能测试: 失败
) else (
    echo 性能测试: 通过
)

if %STRESS_TEST_RESULT% neq 0 (
    echo 压力测试: 失败
) else (
    echo 压力测试: 通过
)

if %FS_TEST_RESULT% neq 0 exit /b %FS_TEST_RESULT%
if %PERF_TEST_RESULT% neq 0 exit /b %PERF_TEST_RESULT%
if %STRESS_TEST_RESULT% neq 0 exit /b %STRESS_TEST_RESULT%

echo 所有测试通过！
exit /b 0