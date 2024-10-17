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
    heartbeat.c
    crypto.c
    access.c
    msg.c
    cfg_srv.c
    health_srv.c
    va.c
    transport.c
)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_LEGACY adv_legacy.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_EXT adv_ext.c)

zephyr_library_sources_ifdef(CONFIG_BT_SETTINGS settings.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS rpl.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_LOW_POWER lpn.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_FRIEND friend.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROV prov.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROVISIONEE provisionee.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROVISIONER provisioner.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_ADV pb_adv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_GATT_COMMON pb_gatt.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_GATT pb_gatt_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_GATT_CLIENT pb_gatt_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_GATT_CLIENT gatt_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROXY_CLIENT proxy_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_GATT_PROXY proxy_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_GATT proxy_msg.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_CFG_CLI cfg_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_HEALTH_CLI health_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SAR_CFG_CLI sar_cfg_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SAR_CFG_SRV sar_cfg_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SAR_CFG  sar_cfg.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OP_AGG_SRV  op_agg_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OP_AGG_CLI  op_agg_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OP_AGG op_agg.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV  large_comp_data_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI  large_comp_data_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PRIV_BEACON_SRV priv_beacon_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_PRIV_BEACON_CLI priv_beacon_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SELF_TEST test.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_CDB cdb.c)

add_subdirectory_ifdef(CONFIG_BT_MESH_SHELL shell)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_BLOB_SRV blob_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_BLOB_CLI blob_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_BLOB_IO_FLASH blob_io_flash.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_DFU_CLI dfu_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_DFU_SRV dfu_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_DFD_SRV dfd_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_DFU_SLOTS dfu_slot.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_DFU_METADATA dfu_metadata.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_RPR_CLI rpr_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_RPR_SRV rpr_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI od_priv_proxy_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV od_priv_proxy_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SOL_PDU_RPL_CLI sol_pdu_rpl_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV sol_pdu_rpl_srv.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_BRG_CFG_CLI brg_cfg_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_BRG_CFG_SRV brg_cfg_srv.c brg_cfg.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SOLICITATION solicitation.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_STATISTIC statistic.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG delayable_msg.c)

zephyr_library_sources(crypto_psa.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)

zephyr_library_include_directories_ifdef(CONFIG_BUILD_WITH_TFM
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)

zephyr_library_sources_ifdef(CONFIG_BT_TESTING testing.c)
