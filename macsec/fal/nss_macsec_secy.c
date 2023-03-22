/*
 * Copyright (c) 2014, 2018, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include "nss_macsec_types.h"
#include "nss_macsec_os.h"
#include "nss_macsec_secy_tx.h"
#include "nss_macsec_secy_rx.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_mib.h"
#include "nss_macsec_secy.h"
#include "nss_macsec_ops.h"

fal_secy_cfg_t g_secy_cfg[FAL_SECY_ID_NUM_MAX];


u32 nss_macsec_secy_reset(u32 secy_id)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_sw_reset(u32 secy_id)	//tiger: is this API necessary?
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_init(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_init == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_init(secy_id);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_sc_sa_mapping_mode_get(u32 secy_id,
					   fal_sc_sa_mapping_mode_e *pmode)
{
	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM)&& (pmode != NULL));

	*pmode = g_secy_cfg[secy_id].sc_sa_map_mode;

	return ERROR_OK;
}

u32 nss_macsec_secy_sc_sa_mapping_mode_set(u32 secy_id,
					   fal_sc_sa_mapping_mode_e mode)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_sc_sa_mapping_mode_t secy_sc_sa_mapping_mode;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM_MAX));

	if (mode >= FAL_SC_SA_MAP_MAX) {
		return ERROR_PARAM;
	}

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_sc_sa_mapping_mode_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (mode == FAL_SC_SA_MAP_1_1)
				secy_sc_sa_mapping_mode = SECY_SC_SA_MAP_1_1;
			else if (mode == FAL_SC_SA_MAP_1_2)
				secy_sc_sa_mapping_mode = SECY_SC_SA_MAP_1_2;
			else if (mode == FAL_SC_SA_MAP_1_4)
				secy_sc_sa_mapping_mode = SECY_SC_SA_MAP_1_4;
			else
				return ERROR_NOT_SUPPORT;
			rv = secy_drv->secy_sc_sa_mapping_mode_set(secy_id,
				secy_sc_sa_mapping_mode);
			g_secy_cfg[secy_id].sc_sa_map_mode = mode;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_controlled_port_en_get(u32 secy_id, bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_controlled_port_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_controlled_port_en_get(secy_id,
				penable);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_controlled_port_en_set(u32 secy_id, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_controlled_port_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_controlled_port_en_set(secy_id,
			enable);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_ip_version_get(u32 secy_id, char *ver_str, u32 ver_str_len)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_cipher_suite_get(u32 secy_id,
				     fal_cipher_suite_e *p_cipher_suite)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_cipher_suite_t secy_cipher_suite;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
		(p_cipher_suite != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_cipher_suite_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_cipher_suite_get(secy_id,
				&secy_cipher_suite);
			if (secy_cipher_suite == SECY_CIPHER_SUITE_XPN_128)
				*p_cipher_suite =
					FAL_CIPHER_SUITE_AES_GCM_XPN_128;
			else if (secy_cipher_suite == SECY_CIPHER_SUITE_256)
				*p_cipher_suite = FAL_CIPHER_SUITE_AES_GCM_256;
			else if (secy_cipher_suite == SECY_CIPHER_SUITE_128)
				*p_cipher_suite = FAL_CIPHER_SUITE_AES_GCM_128;
			else
				*p_cipher_suite =
					FAL_CIPHER_SUITE_AES_GCM_XPN_256;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_cipher_suite_set(u32 secy_id,
				     fal_cipher_suite_e cipher_suite)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_cipher_suite_t secy_cipher_suite;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_cipher_suite_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (cipher_suite == FAL_CIPHER_SUITE_AES_GCM_XPN_128)
				secy_cipher_suite = SECY_CIPHER_SUITE_XPN_128;
			else if (cipher_suite == FAL_CIPHER_SUITE_AES_GCM_256)
				secy_cipher_suite = SECY_CIPHER_SUITE_256;
			else if (cipher_suite == FAL_CIPHER_SUITE_AES_GCM_128)
				secy_cipher_suite = SECY_CIPHER_SUITE_128;
			else
				secy_cipher_suite = SECY_CIPHER_SUITE_XPN_256;
			rv = secy_drv->secy_cipher_suite_set(secy_id,
				secy_cipher_suite);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_mtu_get(u32 secy_id, u32 *pmtu)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (pmtu != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_mtu_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_mtu_get(secy_id, pmtu);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_mtu_set(u32 secy_id, u32 mtu)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	if (mtu > FAL_TX_RX_MTU_MAX) {
		return ERROR_PARAM;
	}

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_mtu_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_mtu_set(secy_id, mtu);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_id_get(u8 *dev_name, u32 *secy_id)
{
	g_error_t rv;

	rv = nss_macsec_dt_secy_id_get(dev_name, secy_id);
	if (rv == ERROR_OK)
		return rv;

	/* static secy id mapping for ipq806x, secy_id = gmac_id - 1 */
#ifdef MACSEC_IPQ806X_SUPPORT
	if (!strncmp((const char *)dev_name, "eth2", 4)) {
		*secy_id = 1;
		rv = ERROR_OK;
	} else if (!strncmp((const char *)dev_name, "eth3", 4)) {
		*secy_id = 2;
		rv = ERROR_OK;
	} else {
		*secy_id = INVALID_SECY_ID;
		rv = ERROR_NOT_FOUND;
	}
#endif
	return rv;
}

u32 nss_macsec_secy_en_get(u32 secy_id, bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_en_get(secy_id, penable);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_en_set(u32 secy_id, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_en_set(secy_id, enable);

		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_xpn_en_get(u32 secy_id, bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_xpn_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_xpn_en_get(secy_id, penable);
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_xpn_en_set(u32 secy_id, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_xpn_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_xpn_en_set(secy_id, enable);
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_flow_control_en_get(u32 secy_id, bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_flow_control_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_flow_control_en_get(secy_id,
				penable);
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_flow_control_en_set(u32 secy_id, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_flow_control_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_flow_control_en_set(secy_id,
				enable);
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_special_pkt_ctrl_get(u32 secy_id,
	enum fal_packet_type_t packet_type,
	enum fal_packet_action_t *packet_action)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_packet_type_t secy_packet_type;
	enum secy_packet_action_t secy_packet_action;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
		(packet_action != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_special_pkt_ctrl_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (packet_type == FAL_PACKET_STP)
				secy_packet_type = PACKET_STP;
			else if (packet_type == FAL_PACKET_CDP)
				secy_packet_type = PACKET_CDP;
			else if (packet_type == FAL_PACKET_LLDP)
				secy_packet_type = PACKET_LLDP;
			else
				return ERROR_NOT_SUPPORT;

			rv = secy_drv->secy_special_pkt_ctrl_get(secy_id,
				secy_packet_type, &secy_packet_action);
			if (secy_packet_action == PACKET_CRYPTO_OR_DISCARD)
				*packet_action = FAL_PACKET_CRYPTO_OR_DISCARD;
			else if (secy_packet_action == PACKET_CRYPTO_OR_UPLOAD)
				*packet_action = FAL_PACKET_CRYPTO_OR_UPLOAD;
			else if (secy_packet_action == PACKET_PLAIN_OR_DISCARD)
				*packet_action = FAL_PACKET_PLAIN_OR_DISCARD;
			else if (secy_packet_action == PACKET_PLAIN_OR_UPLOAD)
				*packet_action = FAL_PACKET_PLAIN_OR_UPLOAD;
			else
				rv = ERROR_NOT_SUPPORT;
		}
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_special_pkt_ctrl_set(u32 secy_id,
	enum fal_packet_type_t packet_type,
	enum fal_packet_action_t packet_action)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_packet_type_t secy_packet_type;
	enum secy_packet_action_t secy_packet_action;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_special_pkt_ctrl_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (packet_type == FAL_PACKET_STP)
				secy_packet_type = PACKET_STP;
			else if (packet_type == FAL_PACKET_CDP)
				secy_packet_type = PACKET_CDP;
			else if (packet_type == FAL_PACKET_LLDP)
				secy_packet_type = PACKET_LLDP;
			else
				return ERROR_NOT_SUPPORT;

			if (packet_action == FAL_PACKET_CRYPTO_OR_DISCARD)
				secy_packet_action = PACKET_CRYPTO_OR_DISCARD;
			else if (packet_action == FAL_PACKET_CRYPTO_OR_UPLOAD)
				secy_packet_action = PACKET_CRYPTO_OR_UPLOAD;
			else if (packet_action == FAL_PACKET_PLAIN_OR_DISCARD)
				secy_packet_action = PACKET_PLAIN_OR_DISCARD;
			else if (packet_action == FAL_PACKET_PLAIN_OR_UPLOAD)
				secy_packet_action = PACKET_PLAIN_OR_UPLOAD;
			else
				return ERROR_NOT_SUPPORT;

			rv = secy_drv->secy_special_pkt_ctrl_set(secy_id,
				secy_packet_type, secy_packet_action);
		}
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_udf_ethtype_get(u32 secy_id, bool *penable, u16 *type)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_udf_ethtype_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_udf_ethtype_get(secy_id,
				penable, type);
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_udf_ethtype_set(u32 secy_id, bool enable, u16 type)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_udf_ethtype_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_udf_ethtype_set(secy_id,
			enable, type);

	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_loopback_get(u32 secy_id, enum fal_loopback_type_t *type)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_loopback_type_t secy_loopback_type;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (type != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_loopback_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_loopback_get(secy_id,
				&secy_loopback_type);
			if (secy_loopback_type == MACSEC_NORMAL)
				*type = FAL_MACSEC_NORMAL;
			else if (secy_loopback_type == MACSEC_PHY_LB)
				*type = FAL_MACSEC_PHY_LB;
			else if (secy_loopback_type == MACSEC_SWITCH_LB)
				*type = FAL_MACSEC_SWITCH_LB;
			else if (secy_loopback_type == MACSEC_BYPASS)
				*type = FAL_MACSEC_BYPASS;
			else
				rv = ERROR_NOT_SUPPORT;
		}
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

u32 nss_macsec_secy_loopback_set(u32 secy_id, enum fal_loopback_type_t type)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_loopback_type_t secy_loopback_type;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_loopback_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (type == FAL_MACSEC_NORMAL)
				secy_loopback_type = MACSEC_NORMAL;
			else if (type == FAL_MACSEC_PHY_LB)
				secy_loopback_type = MACSEC_PHY_LB;
			else if (type == FAL_MACSEC_SWITCH_LB)
				secy_loopback_type = MACSEC_SWITCH_LB;
			else if (type == FAL_MACSEC_BYPASS)
				secy_loopback_type = MACSEC_BYPASS;
			else
				return ERROR_NOT_SUPPORT;

			rv = secy_drv->secy_loopback_set(secy_id,
				secy_loopback_type);
		}
	} else {
		rv = ERROR_NOT_SUPPORT;
	}

	return rv;
}

