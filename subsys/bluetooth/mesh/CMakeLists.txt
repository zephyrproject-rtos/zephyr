# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_MESH
    main.c
    cfg.c
    adv.c
    beacon.c
    net.c
    subnet.c
    app_keys.c
    transport.c
    rpl.c
    heartbeat.c
    crypto.c
    access.c
    cfg_srv.c
    health_srv.c
)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_LEGACY adv_legacy.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_EXT adv_ext.c)

zephyr_library_sources_ifdef(CONFIG_BT_SETTINGS settings.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_LOW_POWER lpn.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_FRIEND friend.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROV prov.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROV_DEVICE prov_device.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROVISIONER provisioner.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_ADV pb_adv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_GATT pb_gatt.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROXY proxy.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_CFG_CLI cfg_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_HEALTH_CLI health_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SELF_TEST test.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SHELL shell.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_CDB cdb.c)
