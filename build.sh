#!/bin/sh
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/env.sh"

if [ "$1" = "clean" ]; then
    echo "Cleaning all modules..."
    for d in "$HERE"/modules/*/; do
        make -C "$d" clean 2>/dev/null || rm -f "$d"/*.ko "$d"/*.o "$d"/*.mod "$d"/*.mod.c "$d"/*.symvers "$d"/*.order "$d"/.*.cmd 2>/dev/null
    done
fi

for d in "$HERE"/modules/*/; do
  make -C "$d"
  cp "$d"/*.ko "$ROOTFS/root/"
done
cp "$KDIR/drivers/i2c/i2c-stub.ko" "$ROOTFS/root/"
(cd "$ROOTFS" && find . | cpio -o -H newc --owner root:root | gzip > "$INITRAMFS")
echo "OK: initramfs actualizado"
