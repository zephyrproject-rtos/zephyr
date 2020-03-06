/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CONFIG_NET_BUF_USER_DATA_SIZE 4096
#define CONFIG_BT_CTLR_TX_BUFFERS 4
#define CONFIG_BT_CTLR_TX_BUFFER_SIZE 512
#define CONFIG_BT_CTLR_LLCP_CONN 4
#define CONFIG_BT_MAX_CONN 4
#define CONFIG_BT_CTLR_COMPANY_ID 0x1234
#define CONFIG_BT_CTLR_SUBVERSION_NUMBER 0x5678

#define DT_CHOSEN_ZEPHYR_ENTROPY_LABEL "simulator"

#define CONFIG_BT_CTLR_ASSERT_HANDLER y
#define CONFIG_BT_CONN
#define CONFIG_BT_CTLR_DATA_LENGTH y

#ifndef CONFIG_BT_CTLR_DATA_LENGTH_MAX
#define CONFIG_BT_CTLR_DATA_LENGTH_MAX 251
#endif

#include <ztest.h>

/*
 * Unit under testing, included since we want to test internal functions
 */
#include "ull_conn.c"

/*
 * find the correct include file, based on PHY configuration
 */
#ifdef CONFIG_BT_CTLR_PHY
#if (defined CONFIG_BT_CTLR_PHY_2M) && (defined CONFIG_BT_CTLR_PHY_CODED)
#include "dle_all.h"
#endif
#if (defined CONFIG_BT_CTLR_PHY_2M) && (!defined CONFIG_BT_CTLR_PHY_CODED)
#include "dle_2m.h"
#endif
#if (!defined CONFIG_BT_CTLR_PHY_2M) && (defined CONFIG_BT_CTLR_PHY_CODED)
#include "dle_coded.h"
#endif
#if (!defined CONFIG_BT_CTLR_PHY_2M) && (!defined CONFIG_BT_CTLR_PHY_CODED)
#include "dle_none.h"
#endif
#else
#include "dle_no_phy.h"
#endif
#include "dle_pkt_us.h"

static struct ll_conn *conn;
static K_SEM_DEFINE(sem_prio_recv, 0, UINT_MAX);

/*
 * Helper functions
 * mainly introduced to keep linelength below 80 chars
 */
static void helper_loop_dle_time(u16_t fex, u16_t f_set, u16_t i)
{
	u16_t rx_time, tx_time;

#ifdef CONFIG_BT_CTLR_PHY
	u16_t j;

	for (j = 0; j < nr_of_time_elements; j++) {
		conn->default_tx_time = default_time[j];
		dle_max_time_get(conn, &rx_time, &tx_time);
		zassert_equal(tx_time,
			      expected_tx_time[j][i][f_set][fex],
			      "Failure in unit test, %d\n %d\n",
			      tx_time, expected_tx_time[j][i][f_set][fex]);
		zassert_equal(rx_time,
			      expected_rx_time[j][i][f_set][fex],
			      "Failure in unit test, %d\n %d\n",
			      rx_time, expected_rx_time[j][i][f_set][fex]);
	}
#else
	dle_max_time_get(conn, &rx_time, &tx_time);

	zassert_equal(tx_time,
		      expected_tx_time[0][i][f_set][fex],
		      "Failure in unit test, got %d\nexpected %d\n",
		      tx_time, expected_tx_time[0][i][f_set][fex]);
	zassert_equal(rx_time,
		      expected_rx_time[0][i][f_set][fex],
		      "Failure in unit test, got %d\nexpected %d\n",
		      rx_time, expected_rx_time[0][i][f_set][fex]);
#endif
}

static void helper_loop_pkt_us(u16_t phy_counter)
{
	u16_t nr_octets = ARRAY_SIZE(octets);
	u16_t nr_us = ARRAY_SIZE(expected_us);
	u16_t counter;
	u16_t phy;
	u16_t calc_time;

	zassert_equal(nr_octets, nr_us, "Failure in test routine\n");

	phy = phy_to_test[phy_counter];
	for (counter = 0; counter < nr_octets; counter++) {
		calc_time = PKT_US(octets[counter], phy);
		zassert_equal(calc_time,
			      expected_us[counter][phy_counter],
			      "Failure in unit test: got -%d- expected -%d-",
			      calc_time, expected_us[counter][phy_counter]);
	}
}

/*
 * Unit test routines
 */

void test_conn_init(void)
{
	u16_t err_state;

	ll_init(&sem_prio_recv);
	err_state = init_reset();
	zassert_equal(err_state, 0, NULL);

	conn = ll_conn_acquire();
	zassert_not_null(conn, NULL);
}

void test_int_dle_max_time_get(void)
{
	zassert_equal(nr_of_size_elements, OCTETS_TO_TEST,
		      "Failure in test procedure\n");
	zassert_equal(nr_of_features, FEATURES_TO_TEST,
		      "Failure in test procedure\n");

	conn->llcp_feature.features = 0x00;
	conn->common.fex_valid = 0;	/* no feature exchange */

	for (u16_t fex = 0; fex <= 1; fex++) {
		conn->common.fex_valid = fex;

		for (u16_t f_set = 0; f_set < nr_of_features; f_set++) {
			conn->llcp_feature.features = feature[f_set];

			for (u16_t i = 0; i < nr_of_size_elements; i++) {
				conn->default_tx_octets = default_octets[i];
				helper_loop_dle_time(fex, f_set, i);
			}
		}
	}
}

void test_int_pkt_us(void)
{
	u16_t nr_phys = ARRAY_SIZE(phy_to_test);
	u16_t phy_counter;

	zassert_equal(PHYS_TO_TEST, nr_phys, "Failure in test routine\n");

	for (phy_counter = 0; phy_counter < nr_phys; phy_counter++)
		helper_loop_pkt_us(phy_counter);
}

void test_main(void)
{
	ztest_test_suite(test,
			 ztest_unit_test(test_conn_init),
			 ztest_unit_test(test_int_dle_max_time_get),
			 ztest_unit_test(test_int_pkt_us)
		);
	ztest_run_test_suite(test);
}
