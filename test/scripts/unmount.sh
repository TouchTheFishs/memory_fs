#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo "卸载 memory_fs..."
# 使用fusermount命令卸载FUSE文件系统
umount "$SCRIPT_DIR/../mount_point"
sleep 2
# 检查文件系统是否已卸载
if ! mountpoint -q "$SCRIPT_DIR/../mount_point"; then
    echo "卸载成功"
else
    echo "卸载失败"
    echo "请检查是否在正确的挂载点操作"
fi