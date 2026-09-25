# p1-i2c — Embedded Linux Portfolio, project 1/3

Linux 6.12 LTS kernel (arm64) built from scratch, with a minimal BusyBox-based
initramfs, running in QEMU (`-M virt`) from WSL2. Includes I2C driver development
(manual instantiation and Device Tree auto-probing) for a simulated sensor.

## Contents

### Modules (`modules/`)
- `hello` — first module, pr_info on load/unload
- `hello_misc` — character device with a configurable parameter
- `demo_sensor` — I2C driver for a simulated sensor (i2c-stub), exposing readings
  via a manual sysfs attribute and via the standard `hwmon` subsystem, with
  `dev_err_probe`-based error handling
- `demo_dt` — platform driver auto-probed via Device Tree (no manual instantiation)

### Scripts
- `env.sh` — shared build variables (architecture, cross-compiler, paths)
- `build.sh` — builds every module in `modules/*`, copies the `.ko` files (and
  `i2c-stub.ko`) into the rootfs, and repacks the initramfs
- `run.sh` — boots QEMU with the auto-generated Device Tree blob
- `run-dt.sh` — boots QEMU with a custom Device Tree blob (`~/virt-custom.dtb`),
  required for the `demo_dt` auto-probe demo

### Docs
- `docs/` — kept out of this public repo; detailed day-by-day notes live in a
  private companion repository

## Usage

```bash
./build.sh      # build kernel modules and refresh the initramfs
./run.sh        # boot QEMU (Ctrl-A, X to exit)
./run-dt.sh     # boot QEMU with a custom Device Tree blob (for demo_dt)
```

Inside QEMU, typical I2C workflow:

```sh
insmod /root/i2c-stub.ko chip_addr=0x48
i2cset -y 0 0x48 0x00 0x1234 w
insmod /root/demo_sensor.ko
echo demo_sensor 0x48 > /sys/bus/i2c/devices/i2c-0/new_device
cat /sys/class/hwmon/hwmon0/temp1_input
```

Device Tree auto-probe (requires `run-dt.sh`):

```sh
insmod /root/demo_dt.ko
dmesg | tail   # "probed via Device Tree, sensor-id=42" — no manual instantiation needed
```

## Status

- **Week 1** — environment, LTS kernel, simulated I2C bus (i2c-stub), basic modules
- **Week 2** — I2C driver for a simulated sensor: manual instantiation, sysfs
  exposure, migration to `hwmon`, robust error handling (`dev_err_probe`)
- **Week 3** (in progress) — Device Tree: platform driver auto-probing (day 1);
  applying the same pattern to the I2C driver next
