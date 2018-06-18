zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            bt.c
            IFDEF:${CONFIG_BT_CONN}                  gatt.c
            IFDEF:${CONFIG_BT_BREDR}                 bredr.c
            IFDEF:${CONFIG_BT_L2CAP_DYNAMIC_CHANNEL} l2cap.c
            IFDEF:${CONFIG_BT_RFCOMM}                rfcomm.c
            IFDEF:${CONFIG_BT_CTLR}                  ll.c
                                                     ticker.c
)

target_sources(subsys_bluetooth PRIVATE ${PRIVATE_SOURCES})
