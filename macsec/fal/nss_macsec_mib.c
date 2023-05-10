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

//#include "osal_common.h"
#include "nss_macsec_types.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_mib.h"
#include <linux/delay.h>
#include "nss_macsec_ops.h"

u32 nss_macsec_secy_tx_sc_mib_get(u32 secy_id, u32 channel,
				  fal_tx_sc_mib_t *pmib)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_tx_sc_mib_t secy_tx_sc_mib;

	memset(&secy_tx_sc_mib, 0, sizeof(secy_tx_sc_mib));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pmib != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_sc_mib_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_tx_sc_mib_get(secy_id, channel,
				&secy_tx_sc_mib);
			pmib->protected_pkts = secy_tx_sc_mib.protected_pkts;
			pmib->encrypted_pkts = secy_tx_sc_mib.encrypted_pkts;
			pmib->protected_octets =
				secy_tx_sc_mib.protected_octets;
			pmib->encrypted_octets =
				secy_tx_sc_mib.encrypted_octets;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_sa_mib_get(u32 secy_id, u32 channel,
				  u32 an, fal_tx_sa_mib_t *pmib)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_tx_sa_mib_t secy_tx_sa_mib;

	memset(&secy_tx_sa_mib, 0, sizeof(secy_tx_sa_mib));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pmib != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_sa_mib_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_tx_sa_mib_get(secy_id,
				channel, an, &secy_tx_sa_mib);
			pmib->hit_drop_redirect =
				secy_tx_sa_mib.hit_drop_redirect;
			pmib->protected2_pkts =
				secy_tx_sa_mib.protected2_pkts;
			pmib->protected_pkts =
				secy_tx_sa_mib.protected_pkts;
			pmib->encrypted_pkts = secy_tx_sa_mib.encrypted_pkts;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_mib_get(u32 secy_id, fal_tx_mib_t *pmib)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_tx_mib_t secy_tx_mib;

	memset(&secy_tx_mib, 0, sizeof(secy_tx_mib));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (pmib != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_mib_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_tx_mib_get(secy_id, &secy_tx_mib);
			pmib->ctl_pkts = secy_tx_mib.ctl_pkts;
			pmib->unknown_sa_pkts =
				secy_tx_mib.unknown_sa_pkts;
			pmib->untagged_pkts = secy_tx_mib.untagged_pkts;
			pmib->too_long = secy_tx_mib.too_long;
			pmib->ecc_error_pkts = secy_tx_mib.ecc_error_pkts;
			pmib->unctrl_hit_drop_redir_pkts =
				secy_tx_mib.unctrl_hit_drop_redir_pkts;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_mib_get(u32 secy_id, u32 channel,
				  u32 an, fal_rx_sa_mib_t *pmib)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_sa_mib_t secy_rx_sa_mib;

	memset(&secy_rx_sa_mib, 0, sizeof(secy_rx_sa_mib));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pmib != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_mib_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sa_mib_get(secy_id, channel, an,
				&secy_rx_sa_mib);
			pmib->untagged_hit_pkts =
				secy_rx_sa_mib.untagged_hit_pkts;
			pmib->hit_drop_redir_pkts =
				secy_rx_sa_mib.hit_drop_redir_pkts;
			pmib->not_using_sa = secy_rx_sa_mib.not_using_sa;
			pmib->unused_sa = secy_rx_sa_mib.unused_sa;
			pmib->not_valid_pkts = secy_rx_sa_mib.not_valid_pkts;
			pmib->invalid_pkts = secy_rx_sa_mib.invalid_pkts;
			pmib->ok_pkts = secy_rx_sa_mib.ok_pkts;
			pmib->late_pkts = secy_rx_sa_mib.late_pkts;
			pmib->delayed_pkts = secy_rx_sa_mib.delayed_pkts;
			pmib->unchecked_pkts = secy_rx_sa_mib.unchecked_pkts;
			pmib->validated_octets =
				secy_rx_sa_mib.validated_octets;
			pmib->decrypted_octets =
				secy_rx_sa_mib.decrypted_octets;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_mib_get(u32 secy_id, fal_rx_mib_t *pmib)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_mib_t secy_rx_mib;

	memset(&secy_rx_mib, 0, sizeof(secy_rx_mib));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) && (pmib != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_mib_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_mib_get(secy_id, &secy_rx_mib);
			pmib->unctrl_prt_tx_octets =
				secy_rx_mib.unctrl_prt_tx_octets;
			pmib->ctl_pkts = secy_rx_mib.ctl_pkts;
			pmib->tagged_miss_pkts = secy_rx_mib.tagged_miss_pkts;
			pmib->untagged_hit_pkts =
				secy_rx_mib.untagged_hit_pkts;
			pmib->notag_pkts = secy_rx_mib.notag_pkts;
			pmib->untagged_pkts = secy_rx_mib.untagged_pkts;
			pmib->bad_tag_pkts = secy_rx_mib.bad_tag_pkts;
			pmib->no_sci_pkts = secy_rx_mib.no_sci_pkts;
			pmib->unknown_sci_pkts =
				secy_rx_mib.unknown_sci_pkts;
			pmib->ctrl_prt_pass_pkts =
				secy_rx_mib.ctrl_prt_pass_pkts;
			pmib->unctrl_prt_pass_pkts =
				secy_rx_mib.unctrl_prt_pass_pkts;
			pmib->ctrl_prt_fail_pkts =
				secy_rx_mib.ctrl_prt_fail_pkts;
			pmib->unctrl_prt_fail_pkts =
				secy_rx_mib.unctrl_prt_fail_pkts;
			pmib->too_long_packets =
				secy_rx_mib.too_long_packets;
			pmib->igpoc_ctl_pkts = secy_rx_mib.igpoc_ctl_pkts;
			pmib->ecc_error_pkts = secy_rx_mib.ecc_error_pkts;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_mib_clear(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_mib_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_tx_mib_clear(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_sc_mib_clear(u32 secy_id, u32 channel)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_sc_mib_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_tx_sc_mib_clear(secy_id, channel);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_tx_sa_mib_clear(u32 secy_id, u32 channel, u32 an)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_tx_sa_mib_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_tx_sa_mib_clear(secy_id,
				channel, an);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_mib_clear(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_mib_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_mib_clear(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_mib_clear(u32 secy_id, u32 channel, u32 an)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_mib_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_mib_clear(secy_id,
				channel, an);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

