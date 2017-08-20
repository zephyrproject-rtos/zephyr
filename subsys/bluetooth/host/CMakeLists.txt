zephyr_library()
target_link_libraries(subsys__bluetooth__host subsys__bluetooth)

target_sources_ifdef(CONFIG_BT_HCI_RAW          subsys__bluetooth__host PRIVATE hci_raw.c)
target_sources_ifdef(CONFIG_BT_DEBUG_MONITOR    subsys__bluetooth__host PRIVATE monitor.c)
target_sources_ifdef(CONFIG_BT_TINYCRYPT_ECC    subsys__bluetooth__host PRIVATE hci_ecc.c)
target_sources_ifdef(CONFIG_BT_INTERNAL_STORAGE subsys__bluetooth__host PRIVATE storage.c)
target_sources_ifdef(CONFIG_BT_A2DP             subsys__bluetooth__host PRIVATE a2dp.c)
target_sources_ifdef(CONFIG_BT_AVDTP            subsys__bluetooth__host PRIVATE avdtp.c)
target_sources_ifdef(CONFIG_BT_RFCOMM           subsys__bluetooth__host PRIVATE rfcomm.c)

target_sources_ifdef(CONFIG_BT_BREDR subsys__bluetooth__host PRIVATE
  keys_br.c
  l2cap_br.c
  sdp.c
  )

target_sources_ifdef(CONFIG_BT_HFP_HF subsys__bluetooth__host PRIVATE
  hfp_hf.c
  at.c
  )

if(CONFIG_BT_HCI_HOST)
  target_sources(subsys__bluetooth__host PRIVATE
    uuid.c
    hci_core.c
    )
  target_sources_ifdef(CONFIG_BT_HOST_CRYPTO subsys__bluetooth__host PRIVATE
    crypto.c
    )

  if(CONFIG_BT_CONN)
    target_sources(subsys__bluetooth__host PRIVATE
      conn.c
      l2cap.c
      att.c
      gatt.c
      )

    if(CONFIG_BT_SMP)
      target_sources(subsys__bluetooth__host PRIVATE
        smp.c
        keys.c
        )
    else()
      target_sources(subsys__bluetooth__host PRIVATE
        smp_null.c
        )
    endif()
  endif()
endif()

# TODO
# add_subdirectory_ifdef(CONFIG_BT_MESH mesh)
