
qca-macsec-objs +=	$(DIR)/nss_macsec_init.o \
			$(DIR)/fal/nss_macsec_fal_api.o \
			$(DIR)/fal/nss_macsec_interrupt.o \
			$(DIR)/fal/nss_macsec.o \
			$(DIR)/fal/nss_macsec_mib.o \
			$(DIR)/fal/nss_macsec_secy.o \
			$(DIR)/fal/nss_macsec_secy_rx.o \
			$(DIR)/fal/nss_macsec_secy_tx.o \
			$(DIR)/fal/nss_macsec_utility.o \
			$(DIR)/dal/qca808x/qca808x_macsec.o \
			$(DIR)/fal/nss_macsec_ops.o \
			$(DIR)/fal/nss_macsec_offload.o

NSS_MACSEC_INCLUDE += -I$(DIR)/include -I$(DIR)/dal -I$(DIR)/fal -I$(DIR)/dal/qca808x
NSS_MACSEC_INCLUDE += -Wall -Werror
