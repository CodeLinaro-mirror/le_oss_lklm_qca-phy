/*
 * Copyright (c) 2014, 2018-2019 The Linux Foundation. All rights reserved.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/phy.h>
#include <net/sock.h>
#include <asm/io.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/notifier.h>
#include "nss_macsec_sdk_api.h"
#include "nss_macsec_fal_api.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_ops.h"
#include "nss_macsec_offload.h"
#include "nss_macsec_init.h"


#define MACSEC_DEVICE_NUM     MAX_SECY_ID

static struct sock *sdk_nl_sk = NULL;

struct nss_macsec_device_t{
	uint8_t  used;
	struct   phy_device *phy_dev;
};
static struct nss_macsec_device_t nss_macsec_device[MACSEC_DEVICE_NUM];



static unsigned int nss_macsec_msg_handle(void *msg_data, unsigned int *sync)
{
	struct sdk_msg_header *header = (struct sdk_msg_header *)msg_data;
	unsigned short cmd_type = header->cmd_type;
	unsigned int ret = 0;

	*sync = 1;
	switch (cmd_type) {
	case SDK_FAL_CMD:{
			ret = nss_macsec_fal_msg_handle(header);
		}
		break;

	default:
		break;
	}

	return ret;
}

static void nss_macsec_netlink_recv(struct sk_buff *__skb)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sdk_msg_header *header = NULL;
	void *msg_data;
	u32 pid = 0, msg_type = 0, sync = 0, ret = 0, msg_len = 0;

	if ((skb = skb_clone(__skb, GFP_KERNEL)) != NULL) {
		nlh = nlmsg_hdr(skb);
		pid = nlh->nlmsg_pid;
		msg_data = NLMSG_DATA(nlh);
		msg_type = nlh->nlmsg_type;
		msg_len = sizeof(struct nlmsghdr) + sizeof(struct sdk_msg_header);
		header = (struct sdk_msg_header *)msg_data;
		if (header->buf_len > (U32_MAX - msg_len))
			return;
		msg_len += header->buf_len;

		if(skb->len < msg_len) {
			osal_print("Unexpected msg received! skb_len [0x%x] less than 0x%x\n",
				skb->len, msg_len);
			return;
		} else if (msg_type == SDK_CALL_MSG) {
			ret = nss_macsec_msg_handle(msg_data, &sync);
			header->ret = ret;
		} else {
			osal_print("Unexpected msg:0x%x received!\n", msg_type);
			return;
		}

		NETLINK_CB(skb).portid = 0; /* from kernel */
		NETLINK_CB(skb).dst_group = 0;	/* unicast */
		netlink_unicast(sdk_nl_sk, skb, pid, MSG_DONTWAIT);
	}

	return;
}

static int nss_macsec_netlink_init(void)
{
	struct netlink_kernel_cfg cfg = {
			.input = nss_macsec_netlink_recv,
	};

	sdk_nl_sk = netlink_kernel_create(&init_net, NETLINK_SDK, &cfg);
	if (sdk_nl_sk == NULL) {
		osal_print("Create netlink socket fail\n");
		return -ENODEV;
	}

	return 0;
}

g_error_t nss_macsec_dt_secy_id_get(u8 *dev_name, u32 *secy_id)
{
	u32 dev_id = 0;
	struct net_device *netdev = NULL;

	/* get net device by ifname */
	netdev = __dev_get_by_name(&init_net, (const char *)dev_name);
	if (unlikely(!netdev)) {
		*secy_id = INVALID_SECY_ID;
		return ERROR_NOT_FOUND;
	}

	dev_id = nss_get_device_id(netdev->phydev);
	if (INVALID_SECY_ID == dev_id) {
		osal_print("%s: fail to find dev_id! \n", __func__);
		*secy_id = INVALID_SECY_ID;
		return ERROR_NOT_FOUND;
	}

	/* find secy_id */
	*secy_id = dev_id;
	return ERROR_OK;
}

static void nss_macsec_netlink_fini(void)
{
	if (sdk_nl_sk) {
		sock_release(sdk_nl_sk->sk_socket);
		sdk_nl_sk = NULL;
	}
}

static struct phy_device * nss_device_id_to_phydev(uint32_t dev_id)
{
	if (dev_id >= MACSEC_DEVICE_NUM) {
		osal_print("Wrong Device ID!\n");
		return NULL;
	}

	if (1 == nss_macsec_device[dev_id].used)
		return nss_macsec_device[dev_id].phy_dev;

	return NULL;
}

uint32_t nss_get_device_id(struct phy_device *phydev)
{
	uint32_t dev_id = 0;
	uint32_t find_id = INVALID_SECY_ID;

	for (dev_id = 0; dev_id < MACSEC_DEVICE_NUM; dev_id++) {
		if ((1 == nss_macsec_device[dev_id].used) &&
		    (phydev == nss_macsec_device[dev_id].phy_dev)) {
			find_id = dev_id;
			break;
		}
	}
	return find_id;
}

int nss_macsec_phy_mdio_write(u32 dev_id, u32 reg_addr, u16 val)
{
	struct phy_device *phydev;
	int ret = 0;

	phydev = nss_device_id_to_phydev(dev_id);
	if (NULL == phydev)
		return -ENODEV;
        ret = phy_write(phydev, reg_addr, val);
	return ret;
}

int nss_macsec_phy_mdio_read(u32 dev_id, u32 reg_addr, u16 *val)
{
	struct phy_device *phydev;
	int ret = 0;

	phydev = nss_device_id_to_phydev(dev_id);
	if (NULL == phydev)
		return -ENODEV;
	ret = phy_read(phydev, reg_addr);
	if (ret < 0)
		return ret;
	*val = ret;
	return 0;
}

static uint32_t nss_macsec_device_enroll(struct phy_device *phydev)
{
	uint32_t dev_id = 0;
	uint32_t assign_id = INVALID_SECY_ID;

	for (dev_id = 0; dev_id < MACSEC_DEVICE_NUM; dev_id++) {
		if (1 == nss_macsec_device[dev_id].used) {
			if (phydev == nss_macsec_device[dev_id].phy_dev) {
				osal_print("macsec device already exists!\n");
				assign_id = INVALID_SECY_ID;
				break;
			}
		} else {
			if (INVALID_SECY_ID == assign_id) {
				assign_id = dev_id;
			}
		}
	}
	if (INVALID_SECY_ID != assign_id) {
		nss_macsec_device[assign_id].used = 1;
		nss_macsec_device[assign_id].phy_dev = phydev;
	}
	return assign_id;
}

static void nss_macsec_device_unenroll(struct phy_device *phydev)
{
	uint32_t dev_id = 0;
	uint32_t assign_id = INVALID_SECY_ID;

	for (dev_id = 0; dev_id < MACSEC_DEVICE_NUM; dev_id++) {
		if ((1 == nss_macsec_device[dev_id].used)&&
		    (phydev == nss_macsec_device[dev_id].phy_dev)){
			assign_id = dev_id;
			break;
		}
	}
	if (INVALID_SECY_ID != assign_id) {
		nss_macsec_device[assign_id].used = 0;
		nss_macsec_device[assign_id].phy_dev = NULL;
	} else {
		osal_print("Failed to unenroll phydev:%pK!\n", phydev);
	}

	return;
}

int nss_macsec_device_init(struct phy_device *phydev)
{
	uint32_t dev_id = 0;

	dev_id = nss_macsec_device_enroll(phydev);
	if (INVALID_SECY_ID == dev_id)
		return -ENODEV;

	nss_macsec_secy_driver_init(dev_id);
	nss_macsec_offload_init(dev_id, phydev);

	osal_print("%s done!\n", __func__);
	return 0;
}

void nss_macsec_device_cleanup(struct phy_device *phydev)
{
	uint32_t dev_id = 0;

	dev_id = nss_get_device_id(phydev);
	if (INVALID_SECY_ID == dev_id) {
		osal_print("%s: fail to find dev_id! \n", __func__);
		return;
	}

	nss_macsec_secy_driver_cleanup(dev_id);
	nss_macsec_device_unenroll(phydev);
	return;
}

int nss_macsec_init(void)
{
	nss_macsec_netlink_init();
	nss_macsec_mutex_init();

	osal_print("%s done!\n", __func__);
	return 0;
}

void nss_macsec_exit(void)
{
	nss_macsec_netlink_fini();
	nss_macsec_mutex_destroy();

	osal_print("%s success!\n", __func__);
	return;
}

