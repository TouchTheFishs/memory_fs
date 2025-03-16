#!/bin/bash
echo "卸载 memory_fs..."
# 如果存在PID文件，则使用它来终止进程
if [ -f ../memory_fs.pid ]; then
    kill $(cat ../memory_fs.pid)
    rm ../memory_fs.pid
fi
# 使用fusermount命令卸载FUSE文件系统
fusermount -u ../mount_point
sleep 2
echo "FUSE 文件系统已卸载"