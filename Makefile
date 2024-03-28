
PRJ_PATH ?= $(shell pwd)

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

####################################################################
# 			Add All Local Flags
####################################################################


####################################################################
# 			Build Object List
####################################################################

OBJ_LIST += qti_ethphy.o

ifdef CONFIG_MACSEC
obj ?= .

DIR := macsec

include $(obj)/$(DIR)/macsec_obj.mk

ccflags-y += $(patsubst -I%,-I$(obj)/%,$(NSS_MACSEC_INCLUDE))

OBJ_LIST += $(qca-macsec-objs)
endif

####################################################################
# 			Linux Kernel Module
####################################################################
ifdef CONFIG_QCA_PHY
obj-m :=  qca-phy.o

qca-phy-objs := $(OBJ_LIST)
endif


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PRJ_PATH) clean
