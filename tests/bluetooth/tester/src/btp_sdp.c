/* btp_ias.c - Bluetooth IAS Server Tester */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CLASSIC)

#include <zephyr/bluetooth/classic/sdp.h>

#include "btp/btp.h"
#include <zephyr/sys/byteorder.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_sdp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), //35 03
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
            BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) //11 0B
        },
        )
    ),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),//35 10
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),// 35 06
            BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
                BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) // 01 00
            },
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
                BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) // 00 19
            },
            )
        },
        {
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),// 35 06
            BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
                BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) // 00 19
            },
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
                BT_SDP_ARRAY_16(0X0100u) //AVDTP version: 01 00
            },
            )
        },
        )
    ),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), //35 08
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), //35 06
            BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
                BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) //11 0d
            },
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
                BT_SDP_ARRAY_16(0x0103U) //01 03
            },
            )
        },
        )
    ),
    BT_SDP_SERVICE_NAME("A2DPSink"),
    BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

#define SDP_SPP_SERVICE(channel) static struct bt_sdp_attribute spp_##channel##_attrs[] = { \
    BT_SDP_NEW_SERVICE,                                                           \
    BT_SDP_LIST(                                                                  \
        BT_SDP_ATTR_SVCLASS_ID_LIST,                                              \
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),                                     \
        BT_SDP_DATA_ELEM_LIST(                                                    \
        {                                                                         \
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                                      \
            BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)                           \
        },                                                                        \
        )                                                                         \
    ),                                                                            \
    BT_SDP_LIST(                                                                  \
        BT_SDP_ATTR_PROTO_DESC_LIST,                                              \
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),                                    \
        BT_SDP_DATA_ELEM_LIST(                                                    \
        {                                                                         \
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),                                 \
            BT_SDP_DATA_ELEM_LIST(                                                \
            {                                                                     \
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                                  \
                BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)                               \
            },                                                                    \
            )                                                                     \
        },                                                                        \
        {                                                                         \
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),                                 \
            BT_SDP_DATA_ELEM_LIST(                                                \
            {                                                                     \
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                                  \
                BT_SDP_ARRAY_16(BT_UUID_RFCOMM_VAL)                               \
            },                                                                    \
            {                                                                     \
                BT_SDP_TYPE_SIZE(BT_SDP_UINT8),                                   \
                BT_SDP_ARRAY_16(channel)                                          \
            },                                                                    \
            )                                                                     \
        },                                                                        \
        )                                                                         \
    ),                                                                            \
    BT_SDP_LIST(                                                                  \
        BT_SDP_ATTR_PROFILE_DESC_LIST,                                            \
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),                                     \
        BT_SDP_DATA_ELEM_LIST(                                                    \
        {                                                                         \
            BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),                                 \
            BT_SDP_DATA_ELEM_LIST(                                                \
            {                                                                     \
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                                  \
                BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)                       \
            },                                                                    \
            {                                                                     \
                BT_SDP_TYPE_SIZE(BT_SDP_UINT16),                                  \
                BT_SDP_ARRAY_16(0x0102U)                                          \
            },                                                                    \
            )                                                                     \
        },                                                                        \
        )                                                                         \
    ),                                                                            \
    BT_SDP_SERVICE_NAME("COM"#channel),                                           \
}

SDP_SPP_SERVICE(1);
SDP_SPP_SERVICE(2);
SDP_SPP_SERVICE(3);
SDP_SPP_SERVICE(4);
SDP_SPP_SERVICE(5);
SDP_SPP_SERVICE(6);
SDP_SPP_SERVICE(7);
SDP_SPP_SERVICE(8);
SDP_SPP_SERVICE(9);
SDP_SPP_SERVICE(10);
SDP_SPP_SERVICE(11);
SDP_SPP_SERVICE(12);
SDP_SPP_SERVICE(13);
SDP_SPP_SERVICE(14);
SDP_SPP_SERVICE(15);
SDP_SPP_SERVICE(16);

static struct bt_sdp_record spp_rec[] = {
    BT_SDP_RECORD(spp_1_attrs),
    BT_SDP_RECORD(spp_2_attrs),
    BT_SDP_RECORD(spp_3_attrs),
    BT_SDP_RECORD(spp_4_attrs),
    BT_SDP_RECORD(spp_5_attrs),
    BT_SDP_RECORD(spp_6_attrs),
    BT_SDP_RECORD(spp_7_attrs),
    BT_SDP_RECORD(spp_8_attrs),
    BT_SDP_RECORD(spp_9_attrs),
    BT_SDP_RECORD(spp_10_attrs),
    BT_SDP_RECORD(spp_11_attrs),
    BT_SDP_RECORD(spp_12_attrs),
    BT_SDP_RECORD(spp_13_attrs),
    BT_SDP_RECORD(spp_14_attrs),
    BT_SDP_RECORD(spp_15_attrs),
    BT_SDP_RECORD(spp_16_attrs),
};


static struct bt_sdp_attribute bqb_pts_test_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), //35 03
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
            BT_SDP_ARRAY_16(0xBDDB) //BD DB
        },
        )
    ),
};

static struct bt_sdp_record bqb_pts_test_rec = BT_SDP_RECORD(bqb_pts_test_attrs);

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	return BTP_STATUS_UNKNOWN_CMD;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = 0,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
};

uint8_t tester_init_sdp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_SDP, handlers,
					 ARRAY_SIZE(handlers));

    bt_sdp_register_service(&a2dp_sink_rec);

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_sdp(void)
{
	return BTP_STATUS_SUCCESS;
}
#endif  /* CONFIG_BT_CLASSIC */
