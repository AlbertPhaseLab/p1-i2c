#!/bin/sh
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/env.sh"
for d in "$HERE"/modules/*/; do
  make -C "$d"
  cp "$d"/*.ko "$ROOTFS/root/"
done
cp "$KDIR/drivers/i2c/i2c-stub.ko" "$ROOTFS/root/"
(cd "$ROOTFS" && find . | cpio -o -H newc --owner root:root | gzip > "$INITRAMFS")
echo "OK: initramfs actualizado"
