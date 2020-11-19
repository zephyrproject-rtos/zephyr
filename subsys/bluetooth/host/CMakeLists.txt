# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_HCI_RAW          hci_raw.c hci_common.c)
zephyr_library_sources_ifdef(CONFIG_BT_DEBUG_MONITOR    monitor.c)
zephyr_library_sources_ifdef(CONFIG_BT_TINYCRYPT_ECC    hci_ecc.c)
zephyr_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
zephyr_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)
zephyr_library_sources_ifdef(CONFIG_BT_TESTING          testing.c)
zephyr_library_sources_ifdef(CONFIG_BT_SETTINGS         settings.c)
zephyr_library_sources_ifdef(CONFIG_BT_HOST_CCM         aes_ccm.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_BREDR
  keys_br.c
  l2cap_br.c
  sdp.c
  ssp.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )

if(CONFIG_BT_HCI_HOST)
  zephyr_library_sources(
    uuid.c
    addr.c
    buf.c
    hci_core.c
    hci_common.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_HOST_CRYPTO
    crypto.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_ECC
    ecc.c
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

  zephyr_library_sources_ifdef(
    CONFIG_BT_ISO
    iso.c
    )

  if(CONFIG_BT_DF)
    zephyr_library_sources(
      direction.c
      )
  endif()
endif()

if(CONFIG_BT_DEBUG_SMP OR CONFIG_BT_DEBUG_KEYS OR CONFIG_BT_LOG_SNIFFER_INFO)
  message(WARNING "One of these options are enabled:
  CONFIG_BT_DEBUG_SMP CONFIG_BT_DEBUG_KEYS CONFIG_BT_LOG_SNIFFER_INFO.
  Private security keys such as the LTK will be printed out, do not use in
  production."
    )
endif()
if(CONFIG_BT_FIXED_PASSKEY)
  message(WARNING "CONFIG_BT_FIXED_PASSKEY is enabled
  A fixed passkey is easy to deduce during the pairing procedure, do not use in
  production."
    )
endif()
if(CONFIG_BT_OOB_DATA_FIXED)
  message(WARNING "CONFIG_BT_OOB_DATA_FIXED is enabled.
  A hardcoded OOB data set will be stored in the image, do not use in
  production."
    )
endif()
if(CONFIG_BT_USE_DEBUG_KEYS OR CONFIG_BT_STORE_DEBUG_KEYS)
  message(WARNING "One or both these options are enabled:
  CONFIG_BT_USE_DEBUG_KEYS CONFIG_BT_STORE_DEBUG_KEYS.
  A predefined, publicly available keypair intended for testing will be used.
  Do not use in production."
    )
endif()
if(CONFIG_BT_CONN_DISABLE_SECURITY)
  message(WARNING "CONFIG_BT_CONN_DISABLE_SECURITY is enabled.
  Security is disabled for incoming requests for GATT attributes and L2CAP
  channels that would otherwise require encryption/authentication in order to
  be accessed.
  Do not use in production."
    )
endif()
