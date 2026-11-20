/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Build-only checks of the Bluetooth MCUmgr roles. Each scenario enables one role, and the
 * guards below fail the build if the configuration does not have the shape of that role.
 */

#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_TEST_MCUMGR_BT_ROLE_SERVER)

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT)
#error "Expected Kconfig option CONFIG_MCUMGR_TRANSPORT_BT not enabled"
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
#error "Server only must not build the Bluetooth client transport"
#endif

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "Server only must not build the SMP client or transport management"
#endif

#if defined(CONFIG_BT_CENTRAL)
#error "Server only must not need the central role"
#endif

#elif defined(CONFIG_TEST_MCUMGR_BT_ROLE_CLIENT)

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) || !defined(CONFIG_SMP_CLIENT)
#error "Expected Kconfig options CONFIG_MCUMGR_TRANSPORT_BT_CLIENT and CONFIG_SMP_CLIENT"
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_BT)
#error "SMP client only must not build the SMP server transport"
#endif

#if defined(CONFIG_BT_PERIPHERAL)
#error "SMP client only must not need the peripheral role"
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "SMP client only must not need transport management"
#endif

#elif defined(CONFIG_TEST_MCUMGR_BT_ROLE_BOTH)

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT) || !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) ||         \
	!defined(CONFIG_SMP_CLIENT)
#error "Expected the SMP server, the Bluetooth client transport and the SMP client"
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "Server and SMP client must not need transport management"
#endif

#elif defined(CONFIG_TEST_MCUMGR_BT_ROLE_SERVER_BRIDGE)

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT) || !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) ||         \
	!defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "Expected the SMP server, the Bluetooth client transport and bridges"
#endif

#if defined(CONFIG_SMP_CLIENT)
#error "Bridges must not need the SMP client"
#endif

#elif defined(CONFIG_TEST_MCUMGR_BT_ROLE_BRIDGE_OUT)

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) || !defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "Expected the Bluetooth client transport and transport management"
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_BT) || defined(CONFIG_BT_PERIPHERAL)
#error "The outgoing side of bridges must not need the SMP server or the peripheral role"
#endif

#if defined(CONFIG_SMP_CLIENT)
#error "The outgoing side of bridges must not need the SMP client"
#endif

#else
#error "No Bluetooth MCUmgr role selected"
#endif

/* Every role with the client transport gets the reassembly context and the experimental tag */
#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
#if !defined(CONFIG_MCUMGR_TRANSPORT_REASSEMBLY)
#error "CONFIG_MCUMGR_TRANSPORT_BT_CLIENT must select CONFIG_MCUMGR_TRANSPORT_REASSEMBLY"
#endif

#if !defined(CONFIG_EXPERIMENTAL)
#error "CONFIG_MCUMGR_TRANSPORT_BT_CLIENT must select CONFIG_EXPERIMENTAL"
#endif
#endif

/* Links the public API of the role, so a missing definition fails the build */
ZTEST(mcumgr_transport_bt_roles, test_role_api_links)
{
#if defined(CONFIG_MCUMGR_TRANSPORT_BT)
	zassert_not_null(smp_bt_notify, "SMP server API missing");
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) && defined(CONFIG_SMP_CLIENT)
	zassert_not_null(smp_client_bt_attach, "Bluetooth client API missing");
	zassert_not_null(smp_client_bt_detach, "Bluetooth client API missing");
	zassert_false(smp_client_bt_is_attached(), "Attached at boot");
#endif
}

ZTEST_SUITE(mcumgr_transport_bt_roles, NULL, NULL, NULL, NULL, NULL);
