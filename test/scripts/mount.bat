@echo off
echo 挂载 memory_fs...
mkdir ..\mount_point 2>nul
mkdir ..\target_dir 2>nul
start /b ..\..\build\Release\memory_fs.exe ..\mount_point --target ..\target_dir --log_level debug
timeout /t 2 /nobreak > nul
echo FUSE 文件系统已挂载到 ..\mount_point