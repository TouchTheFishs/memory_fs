#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
umount "$SCRIPT_DIR/../mount_point"
echo "挂载 memory_fs..."
mkdir -p "$SCRIPT_DIR/../mount_point"
mkdir -p "$SCRIPT_DIR/../target_dir"
# "$SCRIPT_DIR/../../build/memory_fs" "$SCRIPT_DIR/../mount_point" --target "$SCRIPT_DIR/../target_dir" --log_level debug -f &
"$SCRIPT_DIR/../../build/memory_fs" "$SCRIPT_DIR/../mount_point" --target "$SCRIPT_DIR/../target_dir" &
# 保存进程ID以便后续卸载
echo $! > "$SCRIPT_DIR/../memory_fs.pid"
sleep 2
echo "FUSE 文件系统已挂载到 $SCRIPT_DIR/../mount_point"