# p1-i2c — Embedded Linux Portfolio, project 1/3

Linux 6.12 LTS kernel (arm64) built from scratch, with a minimal BusyBox-based
initramfs, running in QEMU (`-M virt`) from WSL2. Base setup for developing a
kernel driver for an I2C sensor.

## Contents
- `modules/hello` — first module, pr_info on load/unload
- `modules/hello_misc` — character device with a configurable parameter
- `env.sh`, `build.sh`, `run.sh` — build and boot scripts
- `docs/` — process log, day by day

## Usage
```bash
./build.sh   # builds the kernel, modules, and repacks the initramfs
./run.sh     # boots QEMU (Ctrl-A, X to exit)
```

## Status
Week 1 complete: environment, LTS kernel, simulated I2C bus (i2c-stub), and
test modules. Week 2: real driver for an I2C sensor.
