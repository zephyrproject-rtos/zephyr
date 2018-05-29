zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_HCI_RAW          hci_raw.c)
zephyr_library_sources_ifdef(CONFIG_BT_DEBUG_MONITOR    monitor.c)
zephyr_library_sources_ifdef(CONFIG_BT_TINYCRYPT_ECC    hci_ecc.c)
zephyr_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
zephyr_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)
zephyr_library_sources_ifdef(CONFIG_BT_TESTING          testing.c)
zephyr_library_sources_ifdef(CONFIG_BT_SETTINGS         settings.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_BREDR
  keys_br.c
  l2cap_br.c
  sdp.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )

if(CONFIG_BT_HCI_HOST)
  zephyr_library_sources(
    uuid.c
    hci_core.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_HOST_CRYPTO
    crypto.c
    )

  if(CONFIG_BT_CONN)
    zephyr_library_sources(
      conn.c
      l2cap.c
      att.c
      gatt.c
      )

    if(CONFIG_BT_SMP)
      zephyr_library_sources(
        smp.c
        keys.c
        )
    else()
      zephyr_library_sources(
        smp_null.c
        )
    endif()
  endif()
endif()

add_subdirectory_ifdef(CONFIG_BT_MESH mesh)
