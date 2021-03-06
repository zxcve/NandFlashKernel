#!/bin/bash
set -euo pipefail

MOD_NAME=project6.ko
DEV_NAME=/dev/lkp_kv
DEV_MAJOR=100

# try to remove the module in case it is already loaded
rmmod $MOD_NAME &> /dev/null || true

# try to delete the device file in case it already exists
rm -rf $DEV_NAME &> /dev/null || true

insmod $MOD_NAME
mknod $DEV_NAME c $DEV_MAJOR 0
