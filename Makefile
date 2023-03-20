
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

ifndef PRJ_PATH
  PRJ_PATH=$(shell pwd)
endif

####################################################################
# 			Add All Local Flags
####################################################################


####################################################################
# 			Build Object List
####################################################################
OBJ_LIST := qti_ethphy.o

####################################################################
# 			Linux Kernel Module
####################################################################
obj-m :=  qca-phy.o
qca-phy-objs := $(OBJ_LIST)


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) clean
