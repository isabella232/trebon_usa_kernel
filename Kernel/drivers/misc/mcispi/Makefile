#  drivers/misc/mcispi/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#       http://www.samsung.com/
#
# Licensed under GPLv2

obj-m	          := MciSPIDriver.o
MciSPIDriver-objs := MciDriver.o  MciQueue.o MciDriver_LINUX.o MciOsalApi_LINUX.o
MciSPIDriver-objs += MciDriver_SPI_S5PV210_ANDROID.o


all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.* *~ .*.cmd Module.symvers modules.order

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
