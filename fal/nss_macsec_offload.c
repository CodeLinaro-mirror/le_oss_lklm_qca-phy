/*
 * Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
/*
 * Driver for Linux MACsec offload support
 *
 */

#include <linux/phy.h>
#include <linux/version.h>
#include <net/macsec.h>
#include "nss_macsec_utility.h"
#include "nss_macsec_ops.h"
#include "nss_macsec.h"
#include "nss_macsec_mib.h"
#include "nss_macsec_secy.h"
#include "nss_macsec_secy_rx.h"
#include "nss_macsec_secy_tx.h"
#include "nss_macsec_offload.h"


#define INVALID_PHY_ADDR  0xFFFFFFFF

nss_macsec_secy_cfg_t *g_pdev_secy_cfg[FAL_SECY_ID_NUM_MAX];

static u32 nss_get_txsc_idx_from_secy(u32 secy_id, const struct macsec_secy *secy, u32 *txsc_idx)
{
	u32 i;

	if (unlikely(!secy))
		return ERROR_PARAM;

	for (i = 0; i < NSS_SECY_SC_MAX_NUM; i++) {
		if (g_pdev_secy_cfg[secy_id]->secy_txsc[i].sw_secy == secy) {
			*txsc_idx = i;
			return ERROR_OK;
		}
	}
	return ERROR_NOT_FOUND;
}

static u32 nss_get_rxsc_idx_from_rxsc(u32 secy_id, const struct macsec_rx_sc *rxsc, u32 *rxsc_idx)
{
	u32 i;

	if (unlikely(!rxsc))
		return ERROR_PARAM;

	for (i = 0; i < NSS_SECY_SC_MAX_NUM; i++) {
		if (g_pdev_secy_cfg[secy_id]->secy_rxsc[i].rx_sc == rxsc) {
			*rxsc_idx = i;
			return ERROR_OK;
		}
	}
	return ERROR_NOT_FOUND;
}

static u32 nss_secy_id_get(struct phy_device *phydev, u32 *secy_id)
{
	u32 secy_idx, phy_addr = INVALID_PHY_ADDR;

		/* get phy address by phy device */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0))
	phy_addr = phydev->addr;
#else
	phy_addr = phydev->mdio.addr;
#endif

	/* find secy_id by phy address */
	if (phy_addr != INVALID_PHY_ADDR) {
		for (secy_idx = 0; secy_idx < MAX_SECY_ID; secy_idx++) {
			if (phy_addr == secy_id_to_phy_addr(secy_idx)) {
				*secy_id = secy_idx;
				return ERROR_OK;
			}
		}
	}
	/* fail to find secy_id */
	*secy_id = INVALID_SECY_ID;
	return ERROR_NOT_FOUND;
}

static u32 nss_get_available_transmit_sc(u32 secy_id, u32 *channel)
{
	u32 sc_ch = 0;
	bool in_use = FALSE;

	for (sc_ch = 0; sc_ch < FAL_SECY_CHANNEL_NUM(secy_id); sc_ch++) {
		if (nss_macsec_secy_tx_sc_in_used_get(secy_id, sc_ch, &in_use))
			continue;

		if (!in_use) {
			*channel = sc_ch;
		    osal_print("%s: channel=%u\n", __func__, *channel);
			return ERROR_OK;
		}
	}
	return ERROR_NOT_FOUND;
}

static u32 nss_get_available_receive_sc(u32 secy_id, u32 *channel)
{
	u32 sc_ch = 0;
	bool in_use = FALSE;

	for (sc_ch = 0; sc_ch < FAL_SECY_CHANNEL_NUM(secy_id); sc_ch++) {
		if(nss_macsec_secy_rx_sc_in_used_get(secy_id, sc_ch, &in_use))
			continue;

		if (!in_use) {
			*channel = sc_ch;
			osal_print("%s: channel=%u\n", __func__, *channel);
						return ERROR_OK;
		}
	}
	return ERROR_NOT_FOUND;
}

static fal_cipher_suite_e nss_cipher_suite_type_get(struct macsec_secy *secy)
{
	fal_cipher_suite_e cipher_suite;

	switch (secy->key_len) {
	case NSS_GCM_AES_128_SAK_LEN:
		cipher_suite = secy->xpn ? FAL_CIPHER_SUITE_AES_GCM_XPN_128 : FAL_CIPHER_SUITE_AES_GCM_128;
		break;
	case NSS_GCM_AES_256_SAK_LEN:
		cipher_suite = secy->xpn ? FAL_CIPHER_SUITE_AES_GCM_XPN_256 : FAL_CIPHER_SUITE_AES_GCM_256;
		break;
	default:
		cipher_suite = FAL_CIPHER_SUITE_AES_GCM_128;
		break;
	}
	return cipher_suite;
}

static u32 nss_set_txsc(const u32 secy_id, const int channel, struct macsec_secy *secy)
{
	u32 ret = ERROR_OK;
	fal_tx_class_lut_t entry;
	fal_cipher_suite_e cipher_suite;
	u8 psci[FAL_SCI_LEN];
	u8 tci = 0;

	cipher_suite = nss_cipher_suite_type_get(secy);
	ret = nss_macsec_secy_cipher_suite_set(secy_id, cipher_suite);
	if (ret){
		osal_print("%s: fail to secy_cipher_suite_set\n", __func__);
		return ret;
	}

	/* class lut */
	memset(&entry, 0, sizeof(entry));

	entry.valid = 1;
	entry.action = FAL_TX_CLASS_ACTION_FORWARD;
	entry.channel = channel;

	if (secy->tx_sc.encrypt)
		tci |= BIT(1);
	if (secy->tx_sc.scb)
		tci |= BIT(2);
	if (secy->tx_sc.send_sci)
		tci |= BIT(3);
	if (secy->tx_sc.end_station)
		tci |= BIT(4);
	/* The C bit is clear if and only if the Secure Data is
	 * exactly the same as the User Data and the ICV is 16 octets long.
	 */
	if (!(secy->icv_len == 16 && !secy->tx_sc.encrypt))
		tci |= BIT(0);

	memcpy(psci, (u8*)&(secy->sci), FAL_SCI_LEN);

	ret = nss_macsec_secy_tx_class_lut_set(secy_id, channel, &entry);
	if (ret){
		osal_print("%s: fail to secy_tx_class_lut_set\n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_tx_sc_create(secy_id, channel, psci, FAL_SCI_LEN);
	if (ret){
		osal_print("%s: fail to set secy SCI\n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_tx_sc_tci_7_2_set(secy_id, channel, tci);
	if (ret){
		osal_print("%s: fail to set secy TCI\n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_tx_sc_protect_set(secy_id, channel, secy->protect_frames);
	if (ret){
		osal_print("%s: fail to set tx_sc_protect \n", __func__);
		return ret;
	}

	return ERROR_OK;
}

static u32 nss_set_rxsc(const u32 secy_id, const int channel, struct macsec_secy *secy, struct macsec_rx_sc *rx_sc)
{
	u32 ret = ERROR_OK;
	fal_rx_prc_lut_t entry;
	fal_rx_sc_validate_frame_e vf;


	/* rx prc lut */
	memset(&entry, 0, sizeof(fal_rx_prc_lut_t));

	memcpy(entry.sci, (u8*)&(rx_sc->sci), FAL_SCI_LEN);

	entry.sci_mask = 0xf;

	entry.valid = 1;
	entry.channel = channel;
	entry.action = FAL_RX_PRC_ACTION_PROCESS;

	/* rx validate frame  */
	if (secy->validate_frames == MACSEC_VALIDATE_STRICT)
		vf = FAL_RX_SC_VALIDATE_FRAME_STRICT;
	else if (secy->validate_frames == MACSEC_VALIDATE_CHECK)
		vf = FAL_RX_SC_VALIDATE_FRAME_CHECK;
	else
		vf = FAL_RX_SC_VALIDATE_FRAME_DISABLED;

	ret = nss_macsec_secy_rx_prc_lut_set(secy_id, channel, &entry);
	if (ret){
		osal_print("%s: fail to rx_prc_lut_set \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_sc_create(secy_id, channel);
	if (ret){
		osal_print("%s: fail to rx_sc_create \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_sc_validate_frame_set(secy_id, channel, vf);
	if (ret){
		osal_print("%s: fail to rx_sc_validate_frame_set \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_sc_replay_protect_set(secy_id, channel,
											secy->replay_protect);
	if (ret){
		osal_print("%s: fail to rx_sc_replay_protect_set \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_sc_anti_replay_window_set(secy_id, channel,
										    secy->replay_window);
	if (ret){
		osal_print("%s: fail to rx_sc_anti_replay_window_set \n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static u32 nss_update_txsa(const u32 secy_id, const int channel, const unsigned char an, const struct macsec_tx_sa *tx_sa)
{
	u32 ret = ERROR_OK;

	ret = nss_macsec_secy_tx_sa_next_pn_set(secy_id, channel, an, tx_sa->next_pn_halves.lower);
	if (ret){
		osal_print("%s: fail to tx_sa_next_pn_set \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_tx_sa_en_set(secy_id, channel, an, tx_sa->active);
	if (ret){
		osal_print("%s: fail to tx_sa_en_set \n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static u32 nss_update_rxsa(const u32 secy_id, const int channel, const unsigned char an, const struct macsec_rx_sa *rx_sa)
{
	u32 ret = ERROR_OK;

	ret = nss_macsec_secy_rx_sa_next_pn_set(secy_id, channel, an, rx_sa->next_pn_halves.lower);
	if (ret){
		osal_print("%s: fail to rx_sa_next_pn_set \n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_sa_en_set(secy_id, channel, an, rx_sa->active);
	if (ret){
		osal_print("%s: fail to rx_sa_en_set \n", __func__);
		return ret;
	}

	return ERROR_OK;
}

static int nss_mdo_dev_open(struct macsec_context *ctx)
{
	u32 secy_id = 0, ret = ERROR_OK;

	/* No operation to perform before the commit step */
	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_controlled_port_en_set(secy_id, TRUE);
	if (ret) {
		osal_print("%s: fail to enable controlled_port\n", __func__);
	}
	return ret;
}

static int nss_mdo_dev_stop(struct macsec_context *ctx)
{
	u32 secy_id = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_controlled_port_en_set(secy_id, FALSE);
	if (ret) {
		osal_print("%s: fail to disable controlled_port\n", __func__);
	}
	return ret;
}

static int nss_mdo_add_secy(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_rx_ctl_filt_t rx_ctl_filt;
	fal_tx_ctl_filt_t tx_ctl_filt;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	if (!g_pdev_secy_cfg[secy_id]->secy_en){
		/* Enable Secy and Let EAPoL bypass */
		ret = nss_macsec_secy_en_set(secy_id, TRUE);
		if (ret){
			osal_print("%s: fail to Enable Secy\n", __func__);
			return ret;
		}
		ret = nss_macsec_secy_sc_sa_mapping_mode_set(secy_id, FAL_SC_SA_MAP_1_4);
		if (ret){
			osal_print("%s: fail to Set secy_sc_sa_mapping_mode\n", __func__);
			return ret;
		}
		memset(&rx_ctl_filt, 0, sizeof(rx_ctl_filt));
		rx_ctl_filt.bypass = 1;
		rx_ctl_filt.match_type = IG_CTL_COMPARE_ETHER_TYPE;
		rx_ctl_filt.match_mask = 0xffff;
		rx_ctl_filt.ether_type_da_range = 0x888e;
		ret = nss_macsec_secy_rx_ctl_filt_set(secy_id, 0, &rx_ctl_filt);
		if (ret){
			osal_print("%s: fail to rx ctl filt set\n", __func__);
			return ret;
		}
		memset(&tx_ctl_filt, 0, sizeof(tx_ctl_filt));
		tx_ctl_filt.bypass = 1;
		tx_ctl_filt.match_type = EG_CTL_COMPARE_ETHER_TYPE;
		tx_ctl_filt.match_mask = 0xffff;
		tx_ctl_filt.ether_type_da_range = 0x888e;
		ret = nss_macsec_secy_tx_ctl_filt_set(secy_id, 0, &tx_ctl_filt);
		if (ret){
			osal_print("%s: fail to tx ctl filt set\n", __func__);
			return ret;
		}
		g_pdev_secy_cfg[secy_id]->secy_en = TRUE;
	}

	ret = nss_get_available_transmit_sc(secy_id, &channel);
	if ((ret != ERROR_OK) || (channel >= NSS_SECY_SC_MAX_NUM)) {
		osal_print("%s: no avaiable channel\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_tx_sc_confidentiality_offset_set(secy_id, channel,
					NSS_SECY_CONFIDENTIALITY_OFFSET_00);
	if (ret != ERROR_OK) {
		osal_print("%s: fail to secy_tx_sc_confidentiality_offset_set\n", __func__);
		return ret;
	}

	ret = nss_set_txsc(secy_id, channel, ctx->secy);
	if (ret == ERROR_OK) {
		g_pdev_secy_cfg[secy_id]->secy_txsc[channel].sw_secy = ctx->secy;
		g_pdev_secy_cfg[secy_id]->n_secy_txsc += 1;
	} else {
		osal_print("%s: fail to add secy\n", __func__);
	}
	return ret;
}

static int nss_mdo_upd_secy(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
	    //osal_print("%s: not found channel\n", __func__);
	    //Add secy by upd secy OPS
		return nss_mdo_add_secy(ctx);
	}

	ret = nss_set_txsc(secy_id, channel, ctx->secy);
	if (ret) {
		osal_print("%s: fail to update secy\n", __func__);
	}
	return ret;
}

static int nss_mdo_del_secy(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if ((ret != ERROR_OK) || (channel >= NSS_SECY_SC_MAX_NUM)) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_tx_class_lut_clear(secy_id, channel);
	if (ret == ERROR_OK) {
		g_pdev_secy_cfg[secy_id]->secy_txsc[channel].sw_secy = NULL;
		if (g_pdev_secy_cfg[secy_id]->n_secy_txsc >0) {
			g_pdev_secy_cfg[secy_id]->n_secy_txsc -= 1;
		}else {
			osal_print("%s: n_secy_txsc error\n", __func__);
		}
	} else {
		osal_print("%s: fail to del secy\n", __func__);
	}

	if ((0 == g_pdev_secy_cfg[secy_id]->n_secy_txsc)&&
	    (TRUE == g_pdev_secy_cfg[secy_id]->secy_en)) {
		nss_macsec_secy_en_set(secy_id, FALSE);
		nss_macsec_secy_rx_ctl_filt_clear_all(secy_id);
		g_pdev_secy_cfg[secy_id]->secy_en = FALSE;
	}
	return ret;
}

static int nss_mdo_add_txsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_tx_sak_t tx_sak;
	int i;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	memset(&tx_sak, 0, sizeof(fal_tx_sak_t));
	tx_sak.sak_len = ctx->secy->key_len;
	if (ctx->secy->key_len == NSS_GCM_AES_128_SAK_LEN) {
		for (i = 0; i < 16; i++)
		    tx_sak.sak[i] = ctx->sa.key[15 - i];

	} else if (ctx->secy->key_len == NSS_GCM_AES_256_SAK_LEN) {
	 /*
		for (i = 0; i < 16; i++) {
			tx_sak.sak1[i] = ctx->sa.key[15 - i];
			tx_sak.sak[i] = ctx->sa.key[31 - i];
		}
	 */
	} else {
		osal_print("%s: error key_len\n", __func__);
		return ERROR_PARAM;
	}

	ret = nss_macsec_secy_tx_sak_set(secy_id, channel,
						ctx->sa.assoc_num, &tx_sak);
	if (ret){
		osal_print("%s: fail to secy_tx_sak_set \n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_tx_sa_create(secy_id, channel, ctx->sa.assoc_num);
	if (ret){
		osal_print("%s: fail to tx_sa_create \n", __func__);
		return ret;
	}

	ret = nss_update_txsa(secy_id, channel, ctx->sa.assoc_num, ctx->sa.tx_sa);
	if (ret){
		osal_print("%s: fail to nss_set_txsa \n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_upd_txsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_update_txsa(secy_id, channel, ctx->sa.assoc_num, ctx->sa.tx_sa);
	if (ret){
		osal_print("%s: fail to nss_update_txsa\n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_del_txsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_tx_sa_del(secy_id, channel, ctx->sa.assoc_num);
	if (ret){
		osal_print("%s: fail to secy_tx_sa_del\n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_add_rxsc(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_available_receive_sc(secy_id, &channel);
	if ((ret != ERROR_OK) || (channel >= NSS_SECY_SC_MAX_NUM)) {
		osal_print("%s: no avaiable channel\n", __func__);
		return ret;
	}

	ret = nss_set_rxsc(secy_id, channel, ctx->secy, ctx->rx_sc);
	if (ret == ERROR_OK) {
		g_pdev_secy_cfg[secy_id]->secy_rxsc[channel].sw_secy = ctx->secy;
		g_pdev_secy_cfg[secy_id]->secy_rxsc[channel].rx_sc = ctx->rx_sc;
	} else {
		osal_print("%s: fail to add rxsc\n", __func__);
	}
	return ret;
}

static int nss_mdo_upd_rxsc(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, ctx->rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_set_rxsc(secy_id, channel, ctx->secy, ctx->rx_sc);
	if (ret) {
		osal_print("%s: fail to update rxsc\n", __func__);
	}
	return ret;
}

static int nss_mdo_del_rxsc(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, ctx->rx_sc, &channel);
	if ((ret != ERROR_OK) || (channel >= NSS_SECY_SC_MAX_NUM)) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_rx_sc_del(secy_id, channel);
	if (ret != ERROR_OK) {
		osal_print("%s: fail to rx_sc_del\n", __func__);
		return ret;
	}
	ret = nss_macsec_secy_rx_prc_lut_clear(secy_id, channel);
	if (ret == ERROR_OK) {
		g_pdev_secy_cfg[secy_id]->secy_rxsc[channel].sw_secy = NULL;
		g_pdev_secy_cfg[secy_id]->secy_rxsc[channel].rx_sc = NULL;
	} else {
		osal_print("%s: fail to del secy\n", __func__);
	}

	return ret;
}

static int nss_mdo_add_rxsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	const struct macsec_rx_sc *rx_sc = ctx->sa.rx_sa->sc;
	int i;
	fal_rx_sak_t rx_sak;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	memset(&rx_sak, 0, sizeof(fal_rx_sak_t));
	rx_sak.sak_len = ctx->secy->key_len;
	if (ctx->secy->key_len == NSS_GCM_AES_128_SAK_LEN) {
		for (i = 0; i < 16; i++)
			rx_sak.sak[i] = ctx->sa.key[15 - i];

	} else if (ctx->secy->key_len == NSS_GCM_AES_256_SAK_LEN) {
	   /*
		for (i = 0; i < 16; i++) {
			rx_sak.sak1[i] = ctx->sa.key[15 - i];
			rx_sak.sak[i] = ctx->sa.key[31 - i];
		}
	    */
	} else {
		osal_print("%s: error key_len\n", __func__);
		return ERROR_PARAM;
	}

	ret = nss_macsec_secy_rx_sa_create(secy_id, channel, ctx->sa.assoc_num);
	if (ret){
		osal_print("%s: fail to tx_sa_create \n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_rx_sak_set(secy_id, channel, ctx->sa.assoc_num, &rx_sak);
	if (ret){
		osal_print("%s: fail to tx_sa_create \n", __func__);
		return ret;
	}
	ret = nss_update_rxsa(secy_id, channel, ctx->sa.assoc_num, ctx->sa.rx_sa);
	if (ret){
		osal_print("%s: fail to nss_set_txsa \n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_upd_rxsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	const struct macsec_rx_sc *rx_sc = ctx->sa.rx_sa->sc;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_update_rxsa(secy_id, channel, ctx->sa.assoc_num, ctx->sa.rx_sa);
	if (ret){
		osal_print("%s: fail to nss_update_rxsa\n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_del_rxsa(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	const struct macsec_rx_sc *rx_sc = ctx->sa.rx_sa->sc;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found channel\n", __func__);
		return ret;
	}

	ret = nss_macsec_secy_rx_sa_del(secy_id, channel, ctx->sa.assoc_num);
	if (ret){
		osal_print("%s: fail to secy_rx_sa_del\n", __func__);
		return ret;
	}
	return ERROR_OK;
}

static int nss_mdo_get_dev_stats(struct macsec_context *ctx)
{
	u32 secy_id = 0, ret = ERROR_OK;
	fal_rx_mib_t rxmib;
	fal_tx_mib_t txmib;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	memset(&rxmib, 0, sizeof(fal_rx_mib_t));
	memset(&txmib, 0, sizeof(fal_tx_mib_t));

	nss_macsec_secy_rx_mib_get(secy_id, &rxmib);
	nss_macsec_secy_tx_mib_get(secy_id, &txmib);

	ctx->stats.dev_stats->OutPktsUntagged = txmib.untagged_pkts;
	ctx->stats.dev_stats->InPktsUntagged = rxmib.untagged_pkts;
	ctx->stats.dev_stats->OutPktsTooLong = txmib.too_long;
	ctx->stats.dev_stats->InPktsNoTag = rxmib.notag_pkts;
	ctx->stats.dev_stats->InPktsBadTag = rxmib.bad_tag_pkts;
	ctx->stats.dev_stats->InPktsUnknownSCI = rxmib.unknown_sci_pkts;
	ctx->stats.dev_stats->InPktsNoSCI = rxmib.no_sci_pkts;
	ctx->stats.dev_stats->InPktsOverrun = rxmib.too_long_packets;

	return ERROR_OK;
}

static int nss_mdo_get_tx_sc_stats(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_tx_sc_mib_t txscmib;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found tx sc\n", __func__);
		return ret;
	}

	memset(&txscmib, 0, sizeof(fal_tx_sc_mib_t));

	nss_macsec_secy_tx_sc_mib_get(secy_id, channel, &txscmib);

	ctx->stats.tx_sc_stats->OutPktsProtected = txscmib.protected_pkts;
	ctx->stats.tx_sc_stats->OutPktsEncrypted = txscmib.encrypted_pkts;
	ctx->stats.tx_sc_stats->OutOctetsProtected = txscmib.protected_octets;
	ctx->stats.tx_sc_stats->OutOctetsEncrypted = txscmib.encrypted_octets;

	return ERROR_OK;
}

static int nss_mdo_get_tx_sa_stats(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_tx_sa_mib_t txsamib;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_txsc_idx_from_secy(secy_id, ctx->secy, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found tx sc\n", __func__);
		return ret;
	}

	memset(&txsamib, 0, sizeof(fal_tx_sa_mib_t));

	nss_macsec_secy_tx_sa_mib_get(secy_id, channel,
							ctx->sa.assoc_num, &txsamib);

	ctx->stats.tx_sa_stats->OutPktsProtected = txsamib.protected_pkts;
	ctx->stats.tx_sa_stats->OutPktsEncrypted = txsamib.encrypted_pkts;

	return ERROR_OK;
}

static int nss_mdo_get_rx_sc_stats(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_rx_sa_mib_t rxsamib;
	u32 i;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, ctx->rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found rx sc\n", __func__);
		return ret;
	}

	for (i = 0; i < 2; i++) {
		memset(&rxsamib, 0, sizeof(fal_rx_sa_mib_t));
		nss_macsec_secy_rx_sa_mib_get(secy_id, channel, i, &rxsamib);

		ctx->stats.rx_sc_stats->InOctetsValidated += rxsamib.validated_octets;
		ctx->stats.rx_sc_stats->InOctetsDecrypted += rxsamib.decrypted_octets;
		ctx->stats.rx_sc_stats->InPktsUnchecked += rxsamib.unchecked_pkts;
		ctx->stats.rx_sc_stats->InPktsDelayed += rxsamib.delayed_pkts;
		ctx->stats.rx_sc_stats->InPktsOK += rxsamib.ok_pkts;
		ctx->stats.rx_sc_stats->InPktsInvalid += rxsamib.invalid_pkts;
		ctx->stats.rx_sc_stats->InPktsLate += rxsamib.late_pkts;
		ctx->stats.rx_sc_stats->InPktsNotValid += rxsamib.not_valid_pkts;
		ctx->stats.rx_sc_stats->InPktsNotUsingSA += rxsamib.not_using_sa;
		ctx->stats.rx_sc_stats->InPktsUnusedSA += rxsamib.unused_sa;
	}
	return ERROR_OK;
}

static int nss_mdo_get_rx_sa_stats(struct macsec_context *ctx)
{
	u32 secy_id = 0, channel = 0, ret = ERROR_OK;
	fal_rx_sa_mib_t rxsamib;

	if (ctx->prepare)
		return ERROR_OK;

	if (NULL == ctx->phydev)
		return ERROR_PARAM;

	ret = nss_secy_id_get(ctx->phydev, &secy_id);
	if (ret) {
		osal_print("%s: fail to find secy_id\n", __func__);
		return ret;
	}

	ret = nss_get_rxsc_idx_from_rxsc(secy_id, ctx->rx_sc, &channel);
	if (ret != ERROR_OK) {
		osal_print("%s: not found rx sc\n", __func__);
		return ret;
	}

	memset(&rxsamib, 0, sizeof(fal_rx_sa_mib_t));

	nss_macsec_secy_rx_sa_mib_get(secy_id, channel,
						  ctx->sa.assoc_num, &rxsamib);

	ctx->stats.rx_sa_stats->InPktsOK = rxsamib.ok_pkts;
	ctx->stats.rx_sa_stats->InPktsInvalid = rxsamib.invalid_pkts;
	ctx->stats.rx_sa_stats->InPktsNotValid = rxsamib.not_valid_pkts;
	ctx->stats.rx_sa_stats->InPktsNotUsingSA = rxsamib.not_using_sa;
	ctx->stats.rx_sa_stats->InPktsUnusedSA = rxsamib.unused_sa;

	return ERROR_OK;
}

static const struct macsec_ops nss_macsec_mdo_ops = {
	.mdo_dev_open = nss_mdo_dev_open,
	.mdo_dev_stop = nss_mdo_dev_stop,
	.mdo_add_secy = nss_mdo_add_secy,
	.mdo_upd_secy = nss_mdo_upd_secy,
	.mdo_del_secy = nss_mdo_del_secy,
	.mdo_add_rxsc = nss_mdo_add_rxsc,
	.mdo_upd_rxsc = nss_mdo_upd_rxsc,
	.mdo_del_rxsc = nss_mdo_del_rxsc,
	.mdo_add_rxsa = nss_mdo_add_rxsa,
	.mdo_upd_rxsa = nss_mdo_upd_rxsa,
	.mdo_del_rxsa = nss_mdo_del_rxsa,
	.mdo_add_txsa = nss_mdo_add_txsa,
	.mdo_upd_txsa = nss_mdo_upd_txsa,
	.mdo_del_txsa = nss_mdo_del_txsa,
	.mdo_get_dev_stats = nss_mdo_get_dev_stats,
	.mdo_get_tx_sc_stats = nss_mdo_get_tx_sc_stats,
	.mdo_get_tx_sa_stats = nss_mdo_get_tx_sa_stats,
	.mdo_get_rx_sc_stats = nss_mdo_get_rx_sc_stats,
	.mdo_get_rx_sa_stats = nss_mdo_get_rx_sa_stats,
};

int nss_macsec_offload_init(u32 dev_id, struct phy_device *phydev)
{
#if defined(NETIF_F_HW_MACSEC)
	struct net_device *ndev = NULL;
#endif

	if (dev_id >= FAL_SECY_ID_NUM_MAX) {
		osal_print("%s: error dev_id:%u\n", __func__, dev_id);
		return ERROR_PARAM;
	}
	if (NULL == phydev){
		osal_print("%s: NULL phydev\n", __func__);
		return ERROR_PARAM;
	}

	switch (phydev->phy_id & phydev->drv->phy_id_mask) {
		case PHY_NAPA:
		case PHY_NAPA_1_1:
		{
			g_pdev_secy_cfg[dev_id] = devm_kzalloc(&phydev->mdio.dev,
							       sizeof(nss_macsec_secy_cfg_t), GFP_KERNEL);
			if (NULL == g_pdev_secy_cfg[dev_id]){
				osal_print("%s: devm_kzalloc error\n", __func__);
				return ERROR_RESOURCE;
			}
			phydev->macsec_ops = &nss_macsec_mdo_ops;
#if defined(NETIF_F_HW_MACSEC)
			ndev = phydev->attached_dev;
			if(ndev)
				ndev->features |= NETIF_F_HW_MACSEC;
#endif
		}
		break;
		default:
			osal_print("%s: phy_id: %x", __func__, phydev->phy_id);
		break;
	}

	return ERROR_OK;
}



