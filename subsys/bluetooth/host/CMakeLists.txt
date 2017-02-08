add_library(subsys__bluetooth__host STATIC "")
zephyr_library(subsys__bluetooth__host)

target_sources_ifdef(CONFIG_BT_HCI_HOST subsys__bluetooth__host PRIVATE
  uuid.c
  hci_core.c
)

target_sources_ifdef(CONFIG_BT_HCI_RAW          subsys__bluetooth__host PRIVATE hci_raw.c)
target_sources_ifdef(CONFIG_BT_DEBUG_MONITOR    subsys__bluetooth__host PRIVATE monitor.c)
target_sources_ifdef(CONFIG_BT_TINYCRYPT_ECC    subsys__bluetooth__host PRIVATE hci_ecc.c)
target_sources_ifdef(CONFIG_BT_INTERNAL_STORAGE subsys__bluetooth__host PRIVATE storage.c)

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

  target_sources_ifdef(CONFIG_BT_BREDR subsys__bluetooth__host PRIVATE
    keys_br.c
    l2cap_br.c
    avdtp.c
    sdp.c
  )

  target_sources_ifdef(CONFIG_BT_HFP_HF subsys__bluetooth__host PRIVATE
    hfp_hf.c
    at.c
  )

  target_sources_ifdef(CONFIG_BT_RFCOMM subsys__bluetooth__host PRIVATE
    rfcomm.c
    avdtp.c
    a2dp.c
  )
endif()

target_link_libraries(subsys__bluetooth__host subsys__bluetooth)
