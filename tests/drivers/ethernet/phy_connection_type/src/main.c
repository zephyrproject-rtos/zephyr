/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Check that the defines in phy_connection_type.h match the enum order of
 * phy-connection-type in ethernet-controller.yaml. Values appended to the
 * binding without a matching define are not detected.
 */

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/ethernet/phy_connection_type.h>
#include <zephyr/ztest.h>

#define CHECK_CONN_TYPE(label, idx)                                                                \
	BUILD_ASSERT(DT_ENUM_IDX(DT_NODELABEL(label), phy_connection_type) == (idx),               \
		     #label " does not match " #idx);                                              \
	zexpect_equal(DT_ENUM_IDX(DT_NODELABEL(label), phy_connection_type), (idx),                \
		      #label " does not match " #idx)

ZTEST(phy_connection_type, test_enum_idx)
{
	CHECK_CONN_TYPE(eth_mii, ETHERNET_PHY_CONN_TYPE_MII);
	CHECK_CONN_TYPE(eth_rmii, ETHERNET_PHY_CONN_TYPE_RMII);
	CHECK_CONN_TYPE(eth_gmii, ETHERNET_PHY_CONN_TYPE_GMII);
	CHECK_CONN_TYPE(eth_rgmii, ETHERNET_PHY_CONN_TYPE_RGMII);
	CHECK_CONN_TYPE(eth_rgmii_id, ETHERNET_PHY_CONN_TYPE_RGMII_ID);
	CHECK_CONN_TYPE(eth_rgmii_txid, ETHERNET_PHY_CONN_TYPE_RGMII_TXID);
	CHECK_CONN_TYPE(eth_rgmii_rxid, ETHERNET_PHY_CONN_TYPE_RGMII_RXID);
	CHECK_CONN_TYPE(eth_tbi, ETHERNET_PHY_CONN_TYPE_TBI);
	CHECK_CONN_TYPE(eth_rtbi, ETHERNET_PHY_CONN_TYPE_RTBI);
	CHECK_CONN_TYPE(eth_smii, ETHERNET_PHY_CONN_TYPE_SMII);
	CHECK_CONN_TYPE(eth_sgmii, ETHERNET_PHY_CONN_TYPE_SGMII);
	CHECK_CONN_TYPE(eth_rev_mii, ETHERNET_PHY_CONN_TYPE_REV_MII);
	CHECK_CONN_TYPE(eth_xgmii, ETHERNET_PHY_CONN_TYPE_XGMII);
	CHECK_CONN_TYPE(eth_moca, ETHERNET_PHY_CONN_TYPE_MOCA);
	CHECK_CONN_TYPE(eth_qsgmii, ETHERNET_PHY_CONN_TYPE_QSGMII);
	CHECK_CONN_TYPE(eth_trgmii, ETHERNET_PHY_CONN_TYPE_TRGMII);
	CHECK_CONN_TYPE(eth_internal, ETHERNET_PHY_CONN_TYPE_INTERNAL);
}

ZTEST_SUITE(phy_connection_type, NULL, NULL, NULL, NULL, NULL);
