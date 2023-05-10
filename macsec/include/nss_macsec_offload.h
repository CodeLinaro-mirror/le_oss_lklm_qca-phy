/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef NSS_MACSEC_OFFLOAD_H
#define NSS_MACSEC_OFFLOAD_H

#include <net/macsec.h>


#define NSS_SECY_SC_MAX_NUM     16
#define NSS_GCM_AES_128_SAK_LEN 16
#define NSS_GCM_AES_256_SAK_LEN 32


typedef struct {
	const struct macsec_secy  *sw_secy;
	const struct macsec_rx_sc *rx_sc;

} nss_macsec_secy_sc_map_data_t;

typedef struct {
	u8   n_secy_txsc;
	bool secy_en;
	nss_macsec_secy_sc_map_data_t secy_txsc[NSS_SECY_SC_MAX_NUM];
	nss_macsec_secy_sc_map_data_t secy_rxsc[NSS_SECY_SC_MAX_NUM];

}nss_macsec_secy_cfg_t;


#endif

