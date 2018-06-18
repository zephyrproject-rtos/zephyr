zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BT_INTERNAL_STORAGE} storage.c
            IFDEF:${CONFIG_BT_HCI_RAW}          hci_raw.c
            IFDEF:${CONFIG_BT_DEBUG_MONITOR}    monitor.c
            IFDEF:${CONFIG_BT_TINYCRYPT_ECC}    hci_ecc.c
            IFDEF:${CONFIG_BT_A2DP}             a2dp.c
            IFDEF:${CONFIG_BT_AVDTP}            avdtp.c
            IFDEF:${CONFIG_BT_RFCOMM}           rfcomm.c
            IFDEF:${CONFIG_BT_TESTING}          testing.c
            IFDEF:${CONFIG_BT_SETTINGS}         settings.c
            IFDEF:${CONFIG_BT_BREDR}            keys_br.c
                                                l2cap_br.c
                                                sdp.c
            IFDEF:${CONFIG_BT_HFP_HF}           hfp_hf.c
                                                at.c
)

if(CONFIG_BT_HCI_HOST)
  zephyr_list(SOURCES APPEND
              OUTPUT PRIVATE_SOURCES
              uuid.c
              hci_core.c
              IFDEF:${CONFIG_BT_HOST_CRYPTO} crypto.c
              IFDEF:${CONFIG_BT_CONN}        conn.c
                                             l2cap.c
                                             att.c
                                             gatt.c
  )

  if(CONFIG_BT_CONN)
    zephyr_list(SOURCES APPEND
                OUTPUT PRIVATE_SOURCES
                IFDEF:${CONFIG_BT_SMP} smp.c
                                       keys.c
                IFNDEF:${CONFIG_BT_SMP} smp_null.c
    )
  endif()
endif()

# Call the standard CMake function.
target_sources(subsys_bluetooth PRIVATE ${PRIVATE_SOURCES})

if(CONFIG_BT_MESH)
  include_relative(mesh/CMakeLists.txt)
endif()
