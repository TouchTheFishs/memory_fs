#!/bin/bash
echo "挂载 memory_fs..."
mkdir -p ../mount_point
mkdir -p ../target_dir
# 在Linux中使用后台运行方式启动FUSE文件系统
../../build/memory_fs ../mount_point --target ../target_dir --log_level debug &
# 保存进程ID以便后续卸载
echo $! > ../memory_fs.pid
sleep 2
echo "FUSE 文件系统已挂载到 ../mount_point"