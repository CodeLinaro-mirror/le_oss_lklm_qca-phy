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

#include "nss_macsec_types.h"
#include "nss_macsec.h"
#include "nss_macsec_secy.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_secy_rx.h"
#include "nss_macsec_ops.h"

u32 nss_macsec_secy_rx_reg_get(u32 secy_id, u32 addr, u32 *pvalue)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_reg_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_reg_get(secy_id, addr,
				(u16 *)pvalue);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_reg_set(u32 secy_id, u32 addr, u32 value)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_reg_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_reg_set(secy_id, addr,
				(u16)value);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_ctl_filt_get(u32 secy_id, u32 filt_id,
				    fal_rx_ctl_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_ctl_filt_t secy_ctl_filt;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_CTL_FILT_NUM) && (pfilt != NULL));

	memset(&secy_ctl_filt, 0, sizeof(secy_ctl_filt));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_ctl_filt_get(secy_id, filt_id,
				&secy_ctl_filt);
			pfilt->bypass = secy_ctl_filt.bypass;
			pfilt->match_type =
			(fal_ig_ctl_match_type_e)secy_ctl_filt.match_type;
			pfilt->match_mask = secy_ctl_filt.match_mask;
			pfilt->ether_type_da_range =
				secy_ctl_filt.ether_type_da_range;
			memcpy(pfilt->sa_da_addr, secy_ctl_filt.sa_da_addr,
				sizeof(secy_ctl_filt.sa_da_addr));
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_ctl_filt_set(u32 secy_id, u32 filt_id,
				    fal_rx_ctl_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_ctl_filt_t secy_ctl_filt;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_CTL_FILT_NUM) && (pfilt != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_ctl_filt.bypass = pfilt->bypass;
			secy_ctl_filt.match_type =
				(enum secy_ctl_match_type_t)pfilt->match_type;
			secy_ctl_filt.match_mask = pfilt->match_mask;
			secy_ctl_filt.ether_type_da_range =
				pfilt->ether_type_da_range;
			memcpy(secy_ctl_filt.sa_da_addr, pfilt->sa_da_addr,
				sizeof(pfilt->sa_da_addr));
			rv = secy_drv->secy_rx_ctl_filt_set(secy_id, filt_id,
				&secy_ctl_filt);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_ctl_filt_clear(u32 secy_id, u32 filt_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM)
			&& (filt_id < FAL_RX_CTL_FILT_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_clear == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_ctl_filt_clear(secy_id,
				filt_id);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_ctl_filt_clear_all(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK(secy_id < FAL_SECY_ID_NUM);

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_clear_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_ctl_filt_clear_all(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_prc_lut_get(u32 secy_id, u32 index,
				   fal_rx_prc_lut_t *pentry)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_prc_lut_t secy_rx_prc_lut;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM) && (pentry != NULL));

	memset(pentry, 0, sizeof(fal_rx_prc_lut_t));
	memset(&secy_rx_prc_lut, 0, sizeof(secy_rx_prc_lut));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_prc_lut_get(secy_id, index,
				&secy_rx_prc_lut);

			*pentry = *(fal_rx_prc_lut_t *)&secy_rx_prc_lut;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_prc_lut_set(u32 secy_id, u32 index,
				   fal_rx_prc_lut_t *pentry)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_prc_lut_t *secy_rx_prc_lut = NULL;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM) && (pentry != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_rx_prc_lut = (struct secy_rx_prc_lut_t *)pentry;
			rv = secy_drv->secy_rx_prc_lut_set(secy_id, index,
				secy_rx_prc_lut);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_prc_lut_clear(u32 secy_id, u32 index)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_prc_lut_clear(secy_id, index);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_prc_lut_clear_all(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_clear_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_prc_lut_clear_all(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_create(u32 secy_id, u32 channel)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_create == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_create(secy_id, channel);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_en_get(u32 secy_id, u32 channel, bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_en_get(secy_id, channel,
				penable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_en_set(u32 secy_id, u32 channel, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_en_set(secy_id, channel,
				enable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_del(u32 secy_id, u32 channel)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_del == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_del(secy_id, channel);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_del_all(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK(secy_id < FAL_SECY_ID_NUM);

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_del_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_del_all(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_validate_frame_get(u32 secy_id, u32 channel,
					     fal_rx_sc_validate_frame_e *pmode)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pmode != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_validate_frame_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sc_validate_frame_get(secy_id,
				channel, &secy_vf_mode);
			if (secy_vf_mode == STRICT)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_STRICT;
			else if (secy_vf_mode == CHECKED)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_CHECK;
			else if (secy_vf_mode == DISABLED)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_validate_frame_set(u32 secy_id, u32 channel,
					     fal_rx_sc_validate_frame_e mode)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode = STRICT;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(mode < FAL_RX_SC_VALIDATE_FRAME_MAX));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_validate_frame_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (mode == FAL_RX_SC_VALIDATE_FRAME_STRICT)
				secy_vf_mode = STRICT;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_CHECK)
				secy_vf_mode = CHECKED;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_DISABLED)
				secy_vf_mode = DISABLED;
			else
				return ERROR_NOT_SUPPORT;
			rv = secy_drv->secy_rx_sc_validate_frame_set(secy_id,
				channel, secy_vf_mode);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_replay_protect_get(u32 secy_id, u32 channel,
					     bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_replay_protect_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_replay_protect_get(secy_id,
				channel, penable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_replay_protect_set(u32 secy_id, u32 channel,
					     bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_replay_protect_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_replay_protect_set(secy_id,
				channel, enable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_anti_replay_window_get(u32 secy_id, u32 channel,
						 u32 *pwindow)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pwindow != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_anti_replay_window_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_anti_replay_window_get(
				secy_id, channel, pwindow);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_anti_replay_window_set(u32 secy_id, u32 channel,
						 u32 window)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_anti_replay_window_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_anti_replay_window_set(
				secy_id, channel, window);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_in_used_get(u32 secy_id, u32 channel,
				      bool *p_in_used)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(p_in_used != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_in_used_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_in_used_get(secy_id,
				channel, p_in_used);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_an_roll_over_get(u32 secy_id, u32 channel,
					   bool *penable)
{
	*penable = 0;
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_an_roll_over_set(u32 secy_id, u32 channel,
					   bool enable)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_start_stop_time_get(u32 secy_id, u32 channel,
					      u32 *p_start_time,
					      u32 *p_stop_time)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_start_stop_time_set(u32 secy_id, u32 channel,
					      u32 start_time, u32 stop_time)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_create(u32 secy_id, u32 channel, u32 an)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_create == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_create(secy_id, channel, an);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_en_get(u32 secy_id, u32 channel, u32 an,
				 bool *penable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_en_get(secy_id,
				channel, an, penable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_en_set(u32 secy_id, u32 channel, u32 an, bool enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_en_set(secy_id,
				channel, an, enable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_next_pn_get(u32 secy_id, u32 channel,
				      u32 an, u32 *pnpn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pnpn != NULL));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_pn_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_pn_get(secy_id,
				channel, an, pnpn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_next_pn_set(u32 secy_id, u32 channel,
				      u32 an, u32 next_pn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_pn_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_pn_set(secy_id,
				channel, an, next_pn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_del(u32 secy_id, u32 channel, u32 an)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_del == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_del(secy_id, channel, an);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_del_all(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_del_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_del_all(secy_id);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sak_get(u32 secy_id, u32 channel,
			       u32 an, fal_rx_sak_t *pkey)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sak_t secy_key;

	memset(&secy_key, 0, sizeof(secy_key));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pkey != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sak_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sak_get(secy_id, channel,
				an, &secy_key);
			*pkey = *(fal_rx_sak_t *)&secy_key;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sak_set(u32 secy_id, u32 channel,
			       u32 an, fal_rx_sak_t *pkey)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sak_t *secy_key;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pkey != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sak_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_key = (struct secy_sak_t *)pkey;
			rv = secy_drv->secy_rx_sak_set(secy_id,
				channel, an, secy_key);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_in_used_get(u32 secy_id, u32 channel,
				      u32 an, bool *p_in_used)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(p_in_used != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_in_used_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_in_used_get(secy_id, channel,
				an, p_in_used);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_start_stop_time_get(u32 secy_id, u32 channel, u32 an,
					      u32 *p_start_time,
					      u32 *p_stop_time)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_start_stop_time_set(u32 secy_id, u32 channel, u32 an,
					      u32 start_time, u32 stop_time)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_pn_threshold_get(u32 secy_id, u32 *p_pn_threshold)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_pn_threshold_set(u32 secy_id, u32 pn_threshold)
{
	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_replay_protect_get(u32 secy_id, u32 *enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_replay_protect_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_replay_protect_get(secy_id,
				(bool *)enable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_replay_protect_set(u32 secy_id, u32 enable)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_replay_protect_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_replay_protect_set(secy_id,
				(bool)enable);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_validate_frame_get(u32 secy_id, u32 *mode)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_validate_frame_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_validate_frame_get(secy_id,
				&secy_vf_mode);
			if (secy_vf_mode == STRICT)
				*mode = FAL_RX_SC_VALIDATE_FRAME_STRICT;
			else if (secy_vf_mode == CHECKED)
				*mode = FAL_RX_SC_VALIDATE_FRAME_CHECK;
			else if (secy_vf_mode == DISABLED)
				*mode = FAL_RX_SC_VALIDATE_FRAME_DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_validate_frame_set(u32 secy_id, u32 mode)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode = STRICT;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_validate_frame_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (mode == FAL_RX_SC_VALIDATE_FRAME_STRICT)
				secy_vf_mode = STRICT;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_CHECK)
				secy_vf_mode = CHECKED;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_DISABLED)
				secy_vf_mode = DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;

			rv = secy_drv->secy_rx_validate_frame_set(secy_id,
				secy_vf_mode);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_sa_next_xpn_get(u32 secy_id, u32 channel,
	u32 an, u32 *p_next_xpn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(p_next_xpn != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_xpn_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_xpn_get(secy_id,
				channel, an, p_next_xpn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_next_xpn_set(u32 secy_id, u32 channel,
	u32 an, u32 next_xpn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_xpn_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_xpn_set(secy_id,
				channel, an, next_xpn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_sc_ssci_get(u32 secy_id, u32 channel, u32 *pssci)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pssci != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_ssci_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_ssci_get(secy_id, channel,
				pssci);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_ssci_set(u32 secy_id, u32 channel, u32 ssci)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_ssci_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_ssci_set(secy_id, channel,
				ssci);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_ki_get(u32 secy_id, u32 channel,
	u32 an, struct fal_rx_sa_ki_t *ki)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sa_ki_t secy_sa_ki;

	memset(&secy_sa_ki, 0, sizeof(secy_sa_ki));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(ki != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_ki_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sa_ki_get(secy_id, channel,
					an, &secy_sa_ki);
			*ki = *(struct fal_rx_sa_ki_t *)&secy_sa_ki;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;

}
u32 nss_macsec_secy_rx_sa_ki_set(u32 secy_id, u32 channel,
	u32 an, struct fal_rx_sa_ki_t *ki)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sa_ki_t *secy_sa_ki = NULL;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_ki_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_sa_ki = (struct secy_sa_ki_t *)ki;
			rv = secy_drv->secy_rx_sa_ki_set(secy_id, channel,
					an, secy_sa_ki);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_udf_filt_set(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_t filter;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_UDF_FILT_MAX_ID) && (pfilt != NULL));

	memset(&filter, 0, sizeof(filter));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_filt_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			filter.udf_field0 = pfilt->udf_field0;
			filter.udf_field1 = pfilt->udf_field1;
			filter.udf_field2 = pfilt->udf_field2;
			filter.type = (enum secy_udf_filt_type_t)pfilt->type;
			filter.operator =
				(enum secy_udf_filt_op_t)pfilt->operator;
			filter.offset = pfilt->offset;
			filter.mask = pfilt->mask;
			rv = secy_drv->secy_rx_udf_filt_set(secy_id, filt_id,
				&filter);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_filt_get(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_t filter;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_UDF_FILT_MAX_ID) && (pfilt != NULL));

	memset(&filter, 0, sizeof(filter));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_filt_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_udf_filt_get(secy_id, filt_id,
				&filter);
			pfilt->udf_field0 = filter.udf_field0;
			pfilt->udf_field1 = filter.udf_field1;
			pfilt->udf_field2 = filter.udf_field2;
			pfilt->type = (enum fal_rx_udf_filt_type_e)filter.type;
			pfilt->operator =
				(enum fal_rx_udf_filt_op_e)filter.operator;
			pfilt->offset = filter.offset;
			pfilt->mask = filter.mask;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_ufilt_cfg_set(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_cfg_t secy_cfg;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(cfg != NULL));

	memset(&secy_cfg, 0, sizeof(secy_cfg));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_ufilt_cfg_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_cfg.enable = cfg->enable;
			secy_cfg.priority = cfg->priority;
			secy_cfg.inverse = cfg->inverse;
			secy_cfg.pattern0 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern0;
			secy_cfg.pattern1 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern1;
			secy_cfg.pattern2 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern2;
			rv = secy_drv->secy_rx_udf_ufilt_cfg_set(secy_id,
				&secy_cfg);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_ufilt_cfg_get(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_cfg_t secy_cfg;


	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(cfg != NULL));

	memset(&secy_cfg, 0, sizeof(secy_cfg));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_ufilt_cfg_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_udf_ufilt_cfg_get(secy_id,
				&secy_cfg);
			cfg->enable = secy_cfg.enable;
			cfg->priority = secy_cfg.priority;
			cfg->inverse = secy_cfg.inverse;
			cfg->pattern0 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern0;
			cfg->pattern1 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern1;
			cfg->pattern2 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern2;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

