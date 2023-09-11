/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/phy.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool_netlink.h>
#include <linux/bitfield.h>
#include <linux/phylink.h>
#if IS_ENABLED(CONFIG_MACSEC)
#include "macsec/include/nss_macsec_init.h"
#endif
#include "qti_ethphy.h"


static int __maybe_unused qti_phy_debug_reg_write(struct phy_device *phydev,
						  u16 reg, u16 data)
{
	int ret;

	ret = phy_write(phydev, MII_QTI_DEBUG_ADDR, reg);
	if (ret < 0)
		return ret;

	return phy_write(phydev, MII_QTI_DEBUG_DATA, data);
}

static int qti_phy_debug_reg_read(struct phy_device *phydev, u16 reg)
{
	int ret;

	ret = phy_write(phydev, MII_QTI_DEBUG_ADDR, reg);
	if (ret < 0)
		return ret;

	return phy_read(phydev, MII_QTI_DEBUG_DATA);
}

static int qti_phy_debug_reg_mask(struct phy_device *phydev, u16 reg,
				  u16 clear, u16 set)
{
	u16 val;
	int ret;

	ret = qti_phy_debug_reg_read(phydev, reg);
	if (ret < 0)
		return ret;

	val = ret & 0xffff;
	val &= ~clear;
	val |= set;

	return phy_write(phydev, MII_QTI_DEBUG_DATA, val);
}

static int qti_phy_set_wol(struct phy_device *phydev,
			   struct ethtool_wolinfo *wol)
{
	int ret, irq_enabled;
	struct qti_priv *priv = phydev->priv;

	priv->wol = wol->wolopts;

	if (wol->wolopts & WAKE_MAGIC) {
		struct net_device *ndev = phydev->attached_dev;
		const u8 *mac;
		unsigned int i;
		static const unsigned int offsets[] = {
			QTI_PHY_LOC_MAC_ADDR_32_47_OFFSET,
			QTI_PHY_LOC_MAC_ADDR_16_31_OFFSET,
			QTI_PHY_LOC_MAC_ADDR_0_15_OFFSET,
		};

		if (!ndev)
			return -ENODEV;

		mac = (const u8 *) ndev->dev_addr;

		if (!is_valid_ether_addr(mac))
			return -EINVAL;

		for (i = 0; i < 3; i++)
			phy_write_mmd(phydev, MDIO_MMD_PCS, offsets[i],
				      mac[(i * 2) + 1] | (mac[(i * 2)] << 8));

		ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, QTI_PHY_MMD3_WOL_CTRL,
				     0, QTI_PHY_WOL_EN);
		if (ret)
			return ret;

		ret = phy_modify(phydev, MII_QTI_INTR_ENABLE, 0, MII_QTI_INTR_ENABLE_WOL);
		if (ret)
			return ret;
	} else {
		ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, QTI_PHY_MMD3_WOL_CTRL,
				     QTI_PHY_WOL_EN, 0);
		if (ret)
			return ret;

		ret = phy_modify(phydev, MII_QTI_INTR_ENABLE, MII_QTI_INTR_ENABLE_WOL, 0);
		if (ret)
			return ret;
	}

	ret = phy_read(phydev, MII_QTI_INTR_STATUS);
	if (ret < 0)
		return ret;

	/* Check interrupts, only the enabled interrupts can
	 * be passed up to the interrupt PIN.
	 */
	irq_enabled = phy_read(phydev, MII_QTI_INTR_ENABLE);
	if (irq_enabled < 0)
		return irq_enabled;

	irq_enabled &= ~MII_QTI_INTR_ENABLE_WOL;
	if (ret & irq_enabled && !phy_polling_mode(phydev))
		phy_trigger_machine(phydev);

	return 0;
}

static void qti_phy_get_wol(struct phy_device *phydev,
			    struct ethtool_wolinfo *wol)
{
	int value;
	struct qti_priv *priv = phydev->priv;

	wol->supported = WAKE_MAGIC | WAKE_PHY;
	wol->wolopts = (priv->wol & WAKE_PHY) ? WAKE_PHY : 0;

	value = phy_read_mmd(phydev, MDIO_MMD_PCS, QTI_PHY_MMD3_WOL_CTRL);
	if (value < 0)
		return;

	if (value & QTI_PHY_WOL_EN)
		wol->wolopts |= WAKE_MAGIC;
}

static int qti_phy_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct qti_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

#if IS_ENABLED(CONFIG_MACSEC)
	nss_macsec_device_init(phydev);
#endif

	return 0;
}

static void qti_phy_remove(struct phy_device *phydev)
{
#if IS_ENABLED(CONFIG_MACSEC)
	nss_macsec_device_cleanup(phydev);
#endif
	return;
}

static int qti_phy_get_features(struct phy_device *phydev)
{
	int err;

	err = genphy_read_abilities(phydev);
	if (err)
		return err;

	err = phy_read_mmd(phydev, MDIO_MMD_AN, QCA808X_PHY_MMD7_CHIP_TYPE);
	if (err < 0)
		return err;

	/* QCA808X does not support 2.5G capability if the chip type is 1G from
	 * the register MMD7 QCA808X_PHY_MMD7_CHIP_TYPE.
	 */
	if (!(QCA808X_PHY_CHIP_TYPE_1G & err)) {
		err = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_PMA_NG_EXTABLE);
		if (err < 0)
			return err;
		linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported,
				 err & MDIO_PMA_NG_EXTABLE_2_5GBT);
	}

	return 0;
}

static int qti_phy_ack_interrupt(struct phy_device *phydev)
{
	int err;
	/* Clear any pending interrupts */
	err = phy_read(phydev, MII_QTI_INTR_STATUS);

	return (err < 0) ? err : 0;
}

static int qti_phy_config_intr(struct phy_device *phydev)
{
	struct qti_priv *priv = phydev->priv;
	int err;
	int value;

	value = phy_read(phydev, MII_QTI_INTR_ENABLE);

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED) {
		err = qti_phy_ack_interrupt(phydev);
		if (err)
			return err;

		value |= MII_QTI_INTR_ENABLE_LNK_FAST_FAIL;
		value |= MII_QTI_INTR_ENABLE_SPEED_CHANGED;
		value |= MII_QTI_INTR_ENABLE_LINK_FAIL;
		value |= MII_QTI_INTR_ENABLE_LINK_SUCCESS;
		value |= MII_QTI_INTR_ENABLE_WOL;
		if (priv->is_fiber) {
			value |= MII_QTI_INTR_ENABLE_LNK_FAIL_SG;
			value |= MII_QTI_INTR_ENABLE_LNK_SUCCESS_SG;
		}

		err = phy_write(phydev, MII_QTI_INTR_ENABLE, value);
	} else {
		err = phy_write(phydev, MII_QTI_INTR_ENABLE, 0);
		if (err)
			return err;

		err = qti_phy_ack_interrupt(phydev);
	}

	return err;
}

static irqreturn_t qti_phy_handle_interrupt(struct phy_device *phydev)
{
	int irq_status, int_enabled;

	irq_status = phy_read(phydev, MII_QTI_INTR_STATUS);
	if (irq_status < 0) {
		phy_error(phydev);
		return IRQ_NONE;
	}

	int_enabled = phy_read(phydev, MII_QTI_INTR_ENABLE);
	if (int_enabled < 0) {
		phy_error(phydev);
		return IRQ_NONE;
	}

	/* Check if it was one of enabled interrupts */
	if (!(irq_status & int_enabled))
		return IRQ_NONE;

	phy_trigger_machine(phydev);

	return IRQ_HANDLED;
}

static int qti_phy_read_specific_status(struct phy_device *phydev)
{
	int ss;

	ss = phy_read(phydev, MII_QTI_SPECIFIC_STATUS);
	if (ss < 0)
		return ss;

	if (ss & MII_QTI_SS_SPEED_DUPLEX_RESOLVED) {
		int sfc, speed;

		sfc = phy_read(phydev, MII_QTI_SPECIFIC_FUNCTION_CONTROL);
		if (sfc < 0)
			return sfc;

		speed = FIELD_GET(MII_QTI_SS_SPEED_MASK, ss);

		switch (speed) {
		case MII_QTI_SS_SPEED_10:
			phydev->speed = SPEED_10;
			break;
		case MII_QTI_SS_SPEED_100:
			phydev->speed = SPEED_100;
			break;
		case MII_QTI_SS_SPEED_1000:
			phydev->speed = SPEED_1000;
			break;
		case MII_QTI_SS_SPEED_2500:
			phydev->speed = SPEED_2500;
			break;
		}
		if (ss & MII_QTI_SS_DUPLEX)
			phydev->duplex = DUPLEX_FULL;
		else
			phydev->duplex = DUPLEX_HALF;

		if (ss & MII_QTI_SS_MDIX)
			phydev->mdix = ETH_TP_MDI_X;
		else
			phydev->mdix = ETH_TP_MDI;

		switch (FIELD_GET(MII_QTI_SFC_MDI_CROSSOVER_MODE_M, sfc)) {
		case MII_QTI_SFC_MANUAL_MDI:
			phydev->mdix_ctrl = ETH_TP_MDI;
			break;
		case MII_QTI_SFC_MANUAL_MDIX:
			phydev->mdix_ctrl = ETH_TP_MDI_X;
			break;
		case MII_QTI_SFC_AUTOMATIC_CROSSOVER:
			phydev->mdix_ctrl = ETH_TP_MDI_AUTO;
			break;
		}
	}

	return 0;
}

static int qti_phy_config_mdix(struct phy_device *phydev, u8 ctrl)
{
	u16 val;

	switch (ctrl) {
	case ETH_TP_MDI:
		val = MII_QTI_SFC_MANUAL_MDI;
		break;
	case ETH_TP_MDI_X:
		val = MII_QTI_SFC_MANUAL_MDIX;
		break;
	case ETH_TP_MDI_AUTO:
		val = MII_QTI_SFC_AUTOMATIC_CROSSOVER;
		break;
	default:
		return 0;
	}

	return phy_modify_changed(phydev, MII_QTI_SPECIFIC_FUNCTION_CONTROL,
				  MII_QTI_SFC_MDI_CROSSOVER_MODE_M,
				  FIELD_PREP(MII_QTI_SFC_MDI_CROSSOVER_MODE_M, val));
}

static int qti_phy_config_aneg(struct phy_device *phydev)
{
	struct qti_priv *priv = phydev->priv;
	int phy_ctrl = 0;
	int ret;

	ret = qti_phy_config_mdix(phydev, phydev->mdix_ctrl);
	if (ret < 0)
		return ret;

	if (ret == 1) {
		ret = genphy_soft_reset(phydev);
		if (ret < 0)
			return ret;
	}

	if (priv->is_1000basex)
		return genphy_c37_config_aneg(phydev);

	/* Not restart auto-negotiation by setting ret to 0,
	 * when calling __genphy_config_aneg later.
	 */
	ret = 0;

	/* The reg MII_BMCR also needs to be configured for force mode */
	if (phydev->autoneg == AUTONEG_DISABLE)
		genphy_c45_pma_setup_forced(phydev);

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->advertising))
		phy_ctrl = MDIO_AN_10GBT_CTRL_ADV2_5G;

	ret = phy_modify_mmd_changed(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_CTRL,
				     MDIO_AN_10GBT_CTRL_ADV2_5G, phy_ctrl);
	if (ret < 0)
		return ret;

	return __genphy_config_aneg(phydev, ret);
}

static int qti_phy_get_downshift(struct phy_device *phydev, u8 *d)
{
	int val;

	val = phy_read(phydev, MII_QTI_SMART_SPEED);
	if (val < 0)
		return val;

	if (val & MII_QTI_SMART_SPEED_ENABLE)
		*d = FIELD_GET(MII_QTI_SMART_SPEED_RETRY_LIMIT_MASK, val) + 2;
	else
		*d = DOWNSHIFT_DEV_DISABLE;

	return 0;
}

static int qti_phy_set_downshift(struct phy_device *phydev, u8 cnt)
{
	u16 mask, set;
	int ret;

	switch (cnt) {
	case DOWNSHIFT_DEV_DEFAULT_COUNT:
		cnt = QTI_PHY_DEFAULT_DOWNSHIFT;
		fallthrough;
	case QTI_PHY_MIN_DOWNSHIFT ... QTI_PHY_MAX_DOWNSHIFT:
		set = MII_QTI_SMART_SPEED_ENABLE |
		      MII_QTI_SMART_SPEED_BYPASS_TIMER |
		      FIELD_PREP(MII_QTI_SMART_SPEED_RETRY_LIMIT_MASK, cnt - 2);
		mask = MII_QTI_SMART_SPEED_RETRY_LIMIT_MASK;
		break;
	case DOWNSHIFT_DEV_DISABLE:
		set = 0;
		mask = MII_QTI_SMART_SPEED_ENABLE |
		       MII_QTI_SMART_SPEED_BYPASS_TIMER;
		break;
	default:
		return -EINVAL;
	}

	ret = phy_modify_changed(phydev, MII_QTI_SMART_SPEED, mask, set);

	/* need to perform a software reset, use phy_init_hw to
	 * make sure we set the reapply any values which might got
	 * lost during software reset.
	 */
	if (ret == 1)
		ret = phy_init_hw(phydev);

	return ret;
}

static int qti_phy_get_tunable(struct phy_device *phydev,
			       struct ethtool_tunable *tuna, void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return qti_phy_get_downshift(phydev, data);
	default:
		return -EOPNOTSUPP;
	}
}

static int qti_phy_set_tunable(struct phy_device *phydev,
			       struct ethtool_tunable *tuna, const void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return qti_phy_set_downshift(phydev, *(const u8 *)data);
	default:
		return -EOPNOTSUPP;
	}
}

static int qti_phy_cdt_start(struct phy_device *phydev, int pair)
{
	u16 cdt;

	cdt = MII_QTI_CDT_ENABLE_TEST |
	      MII_QTI_CDT_LENGTH_UNIT |
	      MII_QTI_CDT_INTER_CHECK_DIS;

	return phy_write(phydev, MII_QTI_CDT, cdt);
}

static int qti_phy_cdt_wait_for_completion(struct phy_device *phydev)
{
	int val, ret;
	u16 cdt_en;

	cdt_en = MII_QTI_CDT_ENABLE_TEST;

	/* One test run takes about 25ms */
	ret = phy_read_poll_timeout(phydev, MII_QTI_CDT, val,
				    !(val & cdt_en),
				    30000, 100000, true);

	return ret < 0 ? ret : 0;
}

static int qca808x_phy_fast_retrain(struct phy_device *phydev, bool enable)
{
	int ret;

	if (!enable)
		return phy_clear_bits_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_MDIO_MMD1_10GBR_FSRT_CSR,
					  QCA808X_MDIO_MMD1_10GBR_FSRT_ENABLE);

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported)) {
		ret = phy_set_bits_mmd(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_CTRL,
				       QCA808X_MDIO_MMD7_10GBT_CTRL_ADVFSRT2_5G);
		if (ret)
			return ret;

		ret = phy_set_bits_mmd(phydev, MDIO_MMD_AN, QCA808X_MDIO_MMD7_CTRL2,
				       QCA808X_MDIO_MMD7_THP_BP2_5GT);
		if (ret)
			return ret;
	}

	return phy_set_bits_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_MDIO_MMD1_10GBR_FSRT_CSR,
				QCA808X_MDIO_MMD1_10GBR_FSRT_ENABLE);
}

static int qca808x_phy_fast_retrain_config(struct phy_device *phydev)
{
	int ret;

	/* Enable fast retrain */
	ret = qca808x_phy_fast_retrain(phydev, true);
	if (ret)
		return ret;

	phy_write_mmd(phydev, MDIO_MMD_AN, QCA808X_PHY_MMD7_TOP_OPTION1,
		      QCA808X_TOP_OPTION1_DATA);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_20DB,
		      QCA808X_MSE_THRESHOLD_20DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_17DB,
		      QCA808X_MSE_THRESHOLD_17DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_27DB,
		      QCA808X_MSE_THRESHOLD_27DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_28DB,
		      QCA808X_MSE_THRESHOLD_28DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_1,
		      QCA808X_MMD3_DEBUG_1_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_4,
		      QCA808X_MMD3_DEBUG_4_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_5,
		      QCA808X_MMD3_DEBUG_5_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_3,
		      QCA808X_MMD3_DEBUG_3_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_6,
		      QCA808X_MMD3_DEBUG_6_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_2,
		      QCA808X_MMD3_DEBUG_2_VALUE);

	return 0;
}

static int qca808x_phy_ms_seed_set(struct phy_device *phydev)
{
	u16 seed_value = prandom_u32_max(QCA808X_MS_SEED_RANGE);

	return qti_phy_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_LOCAL_SEED,
				      QCA808X_MS_SEED_CFG,
				      FIELD_PREP(QCA808X_MS_SEED_CFG, seed_value));
}

static int qca808x_phy_ms_seed_enable(struct phy_device *phydev, bool enable)
{
	u16 seed_enable = 0;

	if (enable)
		seed_enable = QCA808X_MS_SEED_ENABLE;

	return qti_phy_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_LOCAL_SEED,
				      QCA808X_MS_SEED_ENABLE, seed_enable);
}

static int qca808x_phy_config_init(struct phy_device *phydev)
{
	int ret;

	/* Active adc&vga on 802.3az for the link 1000M and 100M */
	ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_ADDR_CLD_CTRL7,
			     QCA808X_8023AZ_AFE_CTRL_MASK, QCA808X_8023AZ_AFE_EN);
	if (ret)
		return ret;

	/* Adjust the threshold on 802.3az for the link 1000M */
	ret = phy_write_mmd(phydev, MDIO_MMD_PCS,
			    QCA808X_PHY_MMD3_AZ_TRAINING_CTRL, QCA808X_MMD3_AZ_TRAINING_VAL);
	if (ret)
		return ret;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported)) {
		/* Config the fast retrain for the link 2500M */
		ret = qca808x_phy_fast_retrain_config(phydev);
		if (ret)
			return ret;

		/* Configure lower ramdom seed to make phy linked as slave mode */
		ret = qca808x_phy_ms_seed_set(phydev);
		if (ret)
			return ret;

		/* Enable seed */
		ret = qca808x_phy_ms_seed_enable(phydev, true);
		if (ret)
			return ret;
	}

#if IS_ENABLED(CONFIG_MACSEC) && defined(NETIF_F_HW_MACSEC)
	if(phydev->attached_dev)
		phydev->attached_dev->features |= NETIF_F_HW_MACSEC;
#endif
	/* Configure adc threshold as 100mv for the link 10M */
	return qti_phy_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_ADC_THRESHOLD,
				      QCA808X_ADC_THRESHOLD_MASK,
				      QCA808X_ADC_THRESHOLD_100MV);
}

static int qca808x_phy_read_status(struct phy_device *phydev)
{
	int ret;

	ret = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_STAT);
	if (ret < 0)
		return ret;

	linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->lp_advertising,
			 ret & MDIO_AN_10GBT_STAT_LP2_5G);

	ret = genphy_read_status(phydev);
	if (ret)
		return ret;

	ret = qti_phy_read_specific_status(phydev);
	if (ret < 0)
		return ret;

	if (phydev->link) {
		if (phydev->speed == SPEED_2500)
			phydev->interface = PHY_INTERFACE_MODE_2500BASEX;
		else
			phydev->interface = PHY_INTERFACE_MODE_SGMII;
	} else {
		/* generate seed as a lower random value to make PHY linked as SLAVE
		 * easily, except for master/slave configuration fault detected.
		 * Be aware of the corner case where the link partner is also the
		 * qca8081 PHY and the seed value is configured as the same value,
		 * the link can't be up and no link change occurs.
		 */
		if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported)) {
			if (phydev->master_slave_state == MASTER_SLAVE_STATE_ERR) {
				qca808x_phy_ms_seed_enable(phydev, false);
			} else {
				qca808x_phy_ms_seed_set(phydev);
				qca808x_phy_ms_seed_enable(phydev, true);
			}
		}
	}

	return 0;
}

static int qca808x_phy_soft_reset(struct phy_device *phydev)
{
	int ret;

	ret = genphy_soft_reset(phydev);
	if (ret < 0)
		return ret;

	return qca808x_phy_ms_seed_enable(phydev, true);
}

static bool qca808x_phy_cdt_code_valid(int cdt_code)
{
	switch (cdt_code) {
	case QCA808X_CDT_STATUS_STAT_SHORT:
	case QCA808X_CDT_STATUS_STAT_OPEN:
		return true;
	default:
		return false;
	}
}

static int qca808x_phy_cable_test_result_convert(int cdt_code)
{
	switch (cdt_code) {
	case QCA808X_CDT_STATUS_STAT_NORMAL:
		return ETHTOOL_A_CABLE_RESULT_CODE_OK;
	case QCA808X_CDT_STATUS_STAT_SHORT:
		return ETHTOOL_A_CABLE_RESULT_CODE_SAME_SHORT;
	case QCA808X_CDT_STATUS_STAT_OPEN:
		return ETHTOOL_A_CABLE_RESULT_CODE_OPEN;
	case QCA808X_CDT_STATUS_STAT_FAIL:
	default:
		return ETHTOOL_A_CABLE_RESULT_CODE_UNSPEC;
	}
}

static int qca808x_phy_cable_test_fault_length(struct phy_device *phydev, int pair)
{
	int val;
	u32 cdt_length_reg = 0;

	switch (pair) {
	case ETHTOOL_A_CABLE_PAIR_A:
		cdt_length_reg = QCA808X_MMD3_CDT_DIAG_PAIR_A;
		break;
	case ETHTOOL_A_CABLE_PAIR_B:
		cdt_length_reg = QCA808X_MMD3_CDT_DIAG_PAIR_B;
		break;
	case ETHTOOL_A_CABLE_PAIR_C:
		cdt_length_reg = QCA808X_MMD3_CDT_DIAG_PAIR_C;
		break;
	case ETHTOOL_A_CABLE_PAIR_D:
		cdt_length_reg = QCA808X_MMD3_CDT_DIAG_PAIR_D;
		break;
	default:
		return -EINVAL;
	}

	val = phy_read_mmd(phydev, MDIO_MMD_PCS, cdt_length_reg);
	if (val < 0)
		return val;

	return (FIELD_GET(QCA808X_CDT_DIAG_LENGTH, val) * 824) / 10;
}

static int qca808x_phy_cable_test_start(struct phy_device *phydev)
{
	int ret;

	/* Disable hibernation */
	ret = qti_phy_debug_reg_mask(phydev, QCA808X_DBG_AN_TEST,
				     QCA808X_HIBERNATION_EN, 0);
	if (ret < 0)
		return ret;
	/* force PHY working in MDI mode */
	ret = qti_phy_config_mdix(phydev, ETH_TP_MDI);
	if (ret < 0)
		return ret;

	/* Force 1000base-T needs to configure PMA/PMD and MII_BMCR */
	phydev->duplex = DUPLEX_FULL;
	phydev->speed = SPEED_1000;
	ret = genphy_c45_pma_setup_forced(phydev);
	if (ret < 0)
		return ret;

	ret = genphy_setup_forced(phydev);
	if (ret < 0)
		return ret;

	/* configure the thresholds for open, short, pair ok test */
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x8074, 0xc040);
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x8076, 0xc040);
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x8077, 0xa060);
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x8078, 0xc050);
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x807a, 0xc060);
	phy_write_mmd(phydev, MDIO_MMD_PCS, 0x807e, 0xb060);

	return 0;
}

static int qca808x_phy_cable_test_get_status(struct phy_device *phydev, bool *finished)
{
	int ret, val;
	int pair_a, pair_b, pair_c, pair_d;

	*finished = false;

	ret = qti_phy_cdt_start(phydev, 0);
	if (ret)
		return ret;

	ret = qti_phy_cdt_wait_for_completion(phydev);
	if (ret)
		return ret;

	val = phy_read_mmd(phydev, MDIO_MMD_PCS, QCA808X_MMD3_CDT_STATUS);
	if (val < 0)
		return val;

	pair_a = FIELD_GET(QCA808X_CDT_CODE_PAIR_A, val);
	pair_b = FIELD_GET(QCA808X_CDT_CODE_PAIR_B, val);
	pair_c = FIELD_GET(QCA808X_CDT_CODE_PAIR_C, val);
	pair_d = FIELD_GET(QCA808X_CDT_CODE_PAIR_D, val);

	ethnl_cable_test_result(phydev, ETHTOOL_A_CABLE_PAIR_A,
				qca808x_phy_cable_test_result_convert(pair_a));
	ethnl_cable_test_result(phydev, ETHTOOL_A_CABLE_PAIR_B,
				qca808x_phy_cable_test_result_convert(pair_b));
	ethnl_cable_test_result(phydev, ETHTOOL_A_CABLE_PAIR_C,
				qca808x_phy_cable_test_result_convert(pair_c));
	ethnl_cable_test_result(phydev, ETHTOOL_A_CABLE_PAIR_D,
				qca808x_phy_cable_test_result_convert(pair_d));

	if (qca808x_phy_cdt_code_valid(pair_a))
		ethnl_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_A,
					      qca808x_phy_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_A));
	if (qca808x_phy_cdt_code_valid(pair_b))
		ethnl_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_B,
					      qca808x_phy_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_B));
	if (qca808x_phy_cdt_code_valid(pair_c))
		ethnl_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_C,
					      qca808x_phy_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_C));
	if (qca808x_phy_cdt_code_valid(pair_d))
		ethnl_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_D,
					      qca808x_phy_cable_test_fault_length(phydev, ETHTOOL_A_CABLE_PAIR_D));

	*finished = true;

	return 0;
}

static struct phy_driver qti_phy_drivers[] = {
{
	/* QTI QCA8081 */
	PHY_ID_MATCH_EXACT(QCA8081_PHY_ID),
	.name			= "Qualcomm Technologies, Inc. QCA8081",
	.flags			= PHY_POLL_CABLE_TEST,
	.probe			= qti_phy_probe,
	.remove			= qti_phy_remove,
	.config_intr		= qti_phy_config_intr,
	.handle_interrupt	= qti_phy_handle_interrupt,
	.get_tunable		= qti_phy_get_tunable,
	.set_tunable		= qti_phy_set_tunable,
	.set_wol		= qti_phy_set_wol,
	.get_wol		= qti_phy_get_wol,
	.get_features		= qti_phy_get_features,
	.config_aneg		= qti_phy_config_aneg,
	.suspend		= genphy_suspend,
	.resume			= genphy_resume,
	.read_status		= qca808x_phy_read_status,
	.config_init		= qca808x_phy_config_init,
	.soft_reset		= qca808x_phy_soft_reset,
	.cable_test_start	= qca808x_phy_cable_test_start,
	.cable_test_get_status	= qca808x_phy_cable_test_get_status,
}, };

static int __init phy_module_init(void)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_MACSEC)
	nss_macsec_init();
#endif
	ret = phy_drivers_register(qti_phy_drivers,
				   ARRAY_SIZE(qti_phy_drivers), THIS_MODULE);

	return ret;
}

static void __exit phy_module_cleanup(void)
{
#if IS_ENABLED(CONFIG_MACSEC)
	nss_macsec_exit();
#endif
	phy_drivers_unregister(qti_phy_drivers, ARRAY_SIZE(qti_phy_drivers));
}

module_init(phy_module_init);
module_exit(phy_module_cleanup);

static struct mdio_device_id __maybe_unused qti_tbl[] = {
	{ PHY_ID_MATCH_EXACT(QCA8081_PHY_ID) },
	{ }
};

MODULE_DEVICE_TABLE(mdio, qti_tbl);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. PHY driver");
MODULE_AUTHOR("QTI");
MODULE_LICENSE("Dual BSD/GPL");
