obj-m += demo_sensor_regmap.o
KDIR ?= $(HOME)/linux
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
