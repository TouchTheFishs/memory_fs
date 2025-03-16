@echo off
echo 卸载 memory_fs...
taskkill /f /im memory_fs.exe 2>nul
timeout /t 2 /nobreak > nul
echo FUSE 文件系统已卸载