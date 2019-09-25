/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <mqtt_internal.h>
#include <sys/util.h>	/* for ARRAY_SIZE */
#include <ztest.h>

#define CLIENTID	"zephyr"
#define CLIENTID_LEN	6
#define TOPIC		"sensors"
#define TOPIC_LEN	7
#define WILL_TOPIC	"quitting"
#define WILL_TOPIC_LEN	8
#define WILL_MSG	"bye"
#define WILL_MSG_LEN	3
#define USERNAME	"zephyr1"
#define USERNAME_LEN	7
#define PASSWORD	"password"
#define PASSWORD_LEN	8

#define BUFFER_SIZE 128

static ZTEST_DMEM u8_t rx_buffer[BUFFER_SIZE];
static ZTEST_DMEM u8_t tx_buffer[BUFFER_SIZE];
static ZTEST_DMEM struct mqtt_client client;

static ZTEST_DMEM struct mqtt_topic topic_qos_0 = {
	.qos = 0,
	.topic.utf8 = TOPIC,
	.topic.size = TOPIC_LEN
};
static ZTEST_DMEM struct mqtt_topic topic_qos_1 = {
	.qos = 1,
	.topic.utf8 = TOPIC,
	.topic.size = TOPIC_LEN
};
static ZTEST_DMEM struct mqtt_topic topic_qos_2 = {
	.qos = 2,
	.topic.utf8 = TOPIC,
	.topic.size = TOPIC_LEN
};
static ZTEST_DMEM struct mqtt_topic will_topic_qos_0 = {
	.qos = 0,
	.topic.utf8 = WILL_TOPIC,
	.topic.size = WILL_TOPIC_LEN
};
static ZTEST_DMEM struct mqtt_topic will_topic_qos_1 = {
	.qos = 1,
	.topic.utf8 = WILL_TOPIC,
	.topic.size = WILL_TOPIC_LEN
};
static ZTEST_DMEM struct mqtt_utf8 will_msg = {
	.utf8 = WILL_MSG,
	.size = WILL_MSG_LEN
};
static ZTEST_DMEM struct mqtt_utf8 username = {
	.utf8 = USERNAME,
	.size = USERNAME_LEN
};
static ZTEST_DMEM struct mqtt_utf8 password = {
	.utf8 = PASSWORD,
	.size = PASSWORD_LEN
};

/**
 * @brief MQTT test structure
 */
struct mqtt_test {
	/* test name, for example: "test connect 1" */
	const char *test_name;

	/* cast to something like:
	 * struct mqtt_publish_param *msg_publish =
	 *                                  (struct mqtt_publish_param *)ctx
	 */
	void *ctx;

	/* pointer to the eval routine, for example:
	 * eval_fcn = eval_msg_connect
	 */
	int (*eval_fcn)(struct mqtt_test *);

	/* expected result */
	u8_t *expected;

	/* length of 'expected' */
	u16_t expected_len;
};

/**
 * @brief eval_msg_connect	Evaluate the given mqtt_test against the
 *				connect packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_connect(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_publish	Evaluate the given mqtt_test against the
 *				publish packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_publish(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_subscribe	Evaluate the given mqtt_test against the
 *				subscribe packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_subscribe(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_suback	Evaluate the given mqtt_test against the
 *				suback packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_suback(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_pingreq	Evaluate the given mqtt_test against the
 *				pingreq packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_pingreq(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_puback	Evaluate the given mqtt_test against the
 *				puback routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_puback(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_puback	Evaluate the given mqtt_test against the
 *				pubcomp routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_pubcomp(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_pubrec	Evaluate the given mqtt_test against the
 *				pubrec routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_pubrec(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_pubrel	Evaluate the given mqtt_test against the
 *				pubrel routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_pubrel(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_unsuback	Evaluate the given mqtt_test against the
 *				unsuback routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_unsuback(struct mqtt_test *mqtt_test);

/**
 * @brief eval_msg_disconnect	Evaluate the given mqtt_test against the
 *				disconnect routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_disconnect(struct mqtt_test *mqtt_test);

/**
 * @brief eval_buffers		Evaluate if two given buffers are equal
 * @param [in] buf		Input buffer 1, mostly used as the 'computed'
 *				buffer
 * @param [in] expected		Expected buffer
 * @param [in] len		'expected' len
 * @return			TC_PASS on success
 * @return			TC_FAIL on error and prints both buffers
 */
static int eval_buffers(const struct buf_ctx *buf,
			const u8_t *expected, u16_t len);

/**
 * @brief print_array		Prints the array 'a' of 'size' elements
 * @param a			The array
 * @param size			Array's size
 */
static void print_array(const u8_t *a, u16_t size);

/*
 * MQTT CONNECT msg:
 * Clean session: 1	Client id: [6] 'zephyr'	Will flag: 0
 * Will QoS: 0		Will retain: 0		Will topic: [0]
 * Will msg: [0]	Keep alive: 60		User name: [0]
 * Password: [0]
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors
 */
static ZTEST_DMEM
u8_t connect1[] = {0x10, 0x12, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		   0x04, 0x02, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		   0x70, 0x68, 0x79, 0x72};

static ZTEST_DMEM struct mqtt_client client_connect1 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 0, .will_topic = NULL,
	.will_message = NULL, .user_name = NULL,
	.password = NULL
};

/*
 * MQTT CONNECT msg:
 * Clean session: 1	Client id: [6] 'zephyr'	Will flag: 1
 * Will QoS: 0		Will retain: 0		Will topic: [8] quitting
 * Will msg: [3] bye	Keep alive: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors --will-topic quitting \
 * --will-qos 0 --will-payload bye
 */
static ZTEST_DMEM
u8_t connect2[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		   0x04, 0x06, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		   0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		   0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		   0x62, 0x79, 0x65};

static ZTEST_DMEM struct mqtt_client client_connect2 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 0, .will_topic = &will_topic_qos_0,
	.will_message = &will_msg, .user_name = NULL,
	.password = NULL
};

/*
 * MQTT CONNECT msg:
 * Same message as connect3, but set  Will retain: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors --will-topic quitting \
 * --will-qos 0 --will-payload bye  --will-retain
 */
static ZTEST_DMEM
u8_t connect3[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		   0x04, 0x26, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		   0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		   0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		   0x62, 0x79, 0x65};

static ZTEST_DMEM struct mqtt_client client_connect3 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 1, .will_topic = &will_topic_qos_0,
	.will_message = &will_msg, .user_name = NULL,
	.password = NULL
};

/*
 * MQTT CONNECT msg:
 * Same message as connect3, but set  Will QoS: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye
 */
static ZTEST_DMEM
u8_t connect4[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		   0x04, 0x0e, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		   0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		   0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		   0x62, 0x79, 0x65};

static ZTEST_DMEM struct mqtt_client client_connect4 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 0, .will_topic = &will_topic_qos_1,
	.will_message = &will_msg, .user_name = NULL,
	.password = NULL
};

/*
 * MQTT CONNECT msg:
 * Same message as connect5, but set Will retain: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye --will-retain
 */
static ZTEST_DMEM
u8_t connect5[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		   0x04, 0x2e, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		   0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		   0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		   0x62, 0x79, 0x65};

static ZTEST_DMEM struct mqtt_client client_connect5 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 1, .will_topic = &will_topic_qos_1,
	.will_message = &will_msg, .user_name = NULL,
	.password = NULL
};

/*
 * MQTT CONNECT msg:
 * Same message as connect6, but set username: zephyr1 and password: password
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 60 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye --will-retain -u zephyr1 -P password
 */
static ZTEST_DMEM
u8_t connect6[] = {0x10, 0x34, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		  0x04, 0xee, 0x00, 0x3c, 0x00, 0x06, 0x7a, 0x65,
		  0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		  0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		  0x62, 0x79, 0x65, 0x00, 0x07, 0x7a, 0x65, 0x70,
		  0x68, 0x79, 0x72, 0x31, 0x00, 0x08, 0x70, 0x61,
		  0x73, 0x73, 0x77, 0x6f, 0x72, 0x64};

static ZTEST_DMEM struct mqtt_client client_connect6 = {
	.clean_session = 1, .client_id.utf8 = CLIENTID,
	.client_id.size = CLIENTID_LEN,
	.will_retain = 1, .will_topic = &will_topic_qos_1,
	.will_message = &will_msg, .user_name = &username,
	.password = &password
};

static ZTEST_DMEM
u8_t disconnect1[] = {0xe0, 0x00};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 0, Retain: 0, topic: sensors, message: OK
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 0 -m "OK"
 */
static ZTEST_DMEM
u8_t publish1[] = {0x30, 0x0b, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		   0x6f, 0x72, 0x73, 0x4f, 0x4b};

static ZTEST_DMEM struct mqtt_publish_param msg_publish1 = {
	.dup_flag = 0, .retain_flag = 0, .message_id = 0,
	.message.topic.qos = 0,
	.message.topic.topic.utf8 = TOPIC,
	.message.topic.topic.size = TOPIC_LEN,
	.message.payload.data = (u8_t *)"OK",
	.message.payload.len = 2,
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 0, Retain: 1, topic: sensors, message: OK
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 0 -m "OK" -r
 */
static ZTEST_DMEM
u8_t publish2[] = {0x31, 0x0b, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		   0x6f, 0x72, 0x73, 0x4f, 0x4b};

static ZTEST_DMEM struct mqtt_publish_param msg_publish2 = {
	.dup_flag = 0, .retain_flag = 1, .message_id = 0,
	.message.topic.qos = 0,
	.message.topic.topic.utf8 = TOPIC,
	.message.topic.topic.size = TOPIC_LEN,
	.message.payload.data = (u8_t *)"OK",
	.message.payload.len = 2,
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 1, Retain: 1, topic: sensors, message: OK, pkt_id: 1
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 1 -m "OK" -r
 */
static ZTEST_DMEM
u8_t publish3[] = {0x33, 0x0d, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		   0x6f, 0x72, 0x73, 0x00, 0x01, 0x4f, 0x4b};

static ZTEST_DMEM struct mqtt_publish_param msg_publish3 = {
	.dup_flag = 0, .retain_flag = 1, .message_id = 1,
	.message.topic.qos = 1,
	.message.topic.topic.utf8 = TOPIC,
	.message.topic.topic.size = TOPIC_LEN,
	.message.payload.data = (u8_t *)"OK",
	.message.payload.len = 2,
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 2, Retain: 0, topic: sensors, message: OK, pkt_id: 1
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 2 -m "OK"
 */
static ZTEST_DMEM
u8_t publish4[] = {0x34, 0x0d, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		   0x6f, 0x72, 0x73, 0x00, 0x01, 0x4f, 0x4b};
static ZTEST_DMEM struct mqtt_publish_param msg_publish4 = {
	.dup_flag = 0, .retain_flag = 0, .message_id = 1,
	.message.topic.qos = 2,
	.message.topic.topic.utf8 = TOPIC,
	.message.topic.topic.size = TOPIC_LEN,
	.message.payload.data = (u8_t *)"OK",
	.message.payload.len = 2,
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 0
 */
static ZTEST_DMEM
u8_t subscribe1[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
		     0x6e, 0x73, 0x6f, 0x72, 0x73, 0x00};
static ZTEST_DMEM struct mqtt_subscription_list msg_subscribe1 = {
	.message_id = 1, .list_count = 1, .list = &topic_qos_0
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 1
 */
static ZTEST_DMEM
u8_t subscribe2[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
		     0x6e, 0x73, 0x6f, 0x72, 0x73, 0x01};
static ZTEST_DMEM struct mqtt_subscription_list msg_subscribe2 = {
	.message_id = 1, .list_count = 1, .list = &topic_qos_1
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 2
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 2
 */
static ZTEST_DMEM
u8_t subscribe3[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
		     0x6e, 0x73, 0x6f, 0x72, 0x73, 0x02};
static ZTEST_DMEM struct mqtt_subscription_list msg_subscribe3 = {
	.message_id = 1, .list_count = 1, .list = &topic_qos_2
};

/*
 * MQTT SUBACK msg
 * pkt_id: 1, qos: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 0
 */
static ZTEST_DMEM
u8_t suback1[] = {0x90, 0x03, 0x00, 0x01, 0x00};
static ZTEST_DMEM u8_t data_suback1[] = { MQTT_SUBACK_SUCCESS_QoS_0 };
static ZTEST_DMEM struct mqtt_suback_param msg_suback1 = {
	.message_id = 1, .return_codes.len = 1,
	.return_codes.data = data_suback1
};

/*
 * MQTT SUBACK message
 * pkt_id: 1, qos: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 1
 */
static ZTEST_DMEM
u8_t suback2[] = {0x90, 0x03, 0x00, 0x01, 0x01};
static ZTEST_DMEM u8_t data_suback2[] = { MQTT_SUBACK_SUCCESS_QoS_1 };
static ZTEST_DMEM struct mqtt_suback_param msg_suback2 = {
	.message_id = 1, .return_codes.len = 1,
	.return_codes.data = data_suback2
};

/*
 * MQTT SUBACK message
 * pkt_id: 1, qos: 2
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 2
 */
static ZTEST_DMEM
u8_t suback3[] = {0x90, 0x03, 0x00, 0x01, 0x02};
static ZTEST_DMEM u8_t data_suback3[] = { MQTT_SUBACK_SUCCESS_QoS_2 };
static ZTEST_DMEM struct mqtt_suback_param msg_suback3 = {
	.message_id = 1, .return_codes.len = 1,
	.return_codes.data = data_suback3
};

static ZTEST_DMEM
u8_t pingreq1[] = {0xc0, 0x00};

static ZTEST_DMEM
u8_t puback1[] = {0x40, 0x02, 0x00, 0x01};
static ZTEST_DMEM struct mqtt_puback_param msg_puback1 = {.message_id = 1};

static ZTEST_DMEM
u8_t pubrec1[] = {0x50, 0x02, 0x00, 0x01};
static ZTEST_DMEM struct mqtt_pubrec_param msg_pubrec1 = {.message_id = 1};

static ZTEST_DMEM
u8_t pubrel1[] = {0x62, 0x02, 0x00, 0x01};
static ZTEST_DMEM struct mqtt_pubrel_param msg_pubrel1 = {.message_id = 1};

static ZTEST_DMEM
u8_t pubcomp1[] = {0x70, 0x02, 0x00, 0x01};
static ZTEST_DMEM struct mqtt_pubcomp_param msg_pubcomp1 = {.message_id = 1};

static ZTEST_DMEM
u8_t unsuback1[] = {0xb0, 0x02, 0x00, 0x01};
static ZTEST_DMEM struct mqtt_unsuback_param msg_unsuback1 = {.message_id = 1};

static ZTEST_DMEM
struct mqtt_test mqtt_tests[] = {

	{.test_name = "CONNECT, new session, zeros",
	 .ctx = &client_connect1, .eval_fcn = eval_msg_connect,
	 .expected = connect1, .expected_len = sizeof(connect1)},

	{.test_name = "CONNECT, new session, will",
	 .ctx = &client_connect2, .eval_fcn = eval_msg_connect,
	 .expected = connect2, .expected_len = sizeof(connect2)},

	{.test_name = "CONNECT, new session, will retain",
	 .ctx = &client_connect3, .eval_fcn = eval_msg_connect,
	 .expected = connect3, .expected_len = sizeof(connect3)},

	{.test_name = "CONNECT, new session, will qos = 1",
	 .ctx = &client_connect4, .eval_fcn = eval_msg_connect,
	 .expected = connect4, .expected_len = sizeof(connect4)},

	{.test_name = "CONNECT, new session, will qos = 1, will retain",
	 .ctx = &client_connect5, .eval_fcn = eval_msg_connect,
	 .expected = connect5, .expected_len = sizeof(connect5)},

	{.test_name = "CONNECT, new session, username and password",
	 .ctx = &client_connect6, .eval_fcn = eval_msg_connect,
	 .expected = connect6, .expected_len = sizeof(connect6)},

	{.test_name = "DISCONNECT",
	 .ctx = NULL, .eval_fcn = eval_msg_disconnect,
	 .expected = disconnect1, .expected_len = sizeof(disconnect1)},

	{.test_name = "PUBLISH, qos = 0",
	 .ctx = &msg_publish1, .eval_fcn = eval_msg_publish,
	 .expected = publish1, .expected_len = sizeof(publish1)},

	{.test_name = "PUBLISH, retain = 1",
	 .ctx = &msg_publish2, .eval_fcn = eval_msg_publish,
	 .expected = publish2, .expected_len = sizeof(publish2)},

	{.test_name = "PUBLISH, retain = 1, qos = 1",
	 .ctx = &msg_publish3, .eval_fcn = eval_msg_publish,
	 .expected = publish3, .expected_len = sizeof(publish3)},

	{.test_name = "PUBLISH, qos = 2",
	 .ctx = &msg_publish4, .eval_fcn = eval_msg_publish,
	 .expected = publish4, .expected_len = sizeof(publish4)},

	{.test_name = "SUBSCRIBE, one topic, qos = 0",
	 .ctx = &msg_subscribe1, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe1, .expected_len = sizeof(subscribe1)},

	{.test_name = "SUBSCRIBE, one topic, qos = 1",
	 .ctx = &msg_subscribe2, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe2, .expected_len = sizeof(subscribe2)},

	{.test_name = "SUBSCRIBE, one topic, qos = 2",
	 .ctx = &msg_subscribe3, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe3, .expected_len = sizeof(subscribe3)},

	{.test_name = "SUBACK, one topic, qos = 0",
	 .ctx = &msg_suback1, .eval_fcn = eval_msg_suback,
	 .expected = suback1, .expected_len = sizeof(suback1)},

	{.test_name = "SUBACK, one topic, qos = 1",
	 .ctx = &msg_suback2, .eval_fcn = eval_msg_suback,
	 .expected = suback2, .expected_len = sizeof(suback2)},

	{.test_name = "SUBACK, one topic, qos = 2",
	 .ctx = &msg_suback3, .eval_fcn = eval_msg_suback,
	 .expected = suback3, .expected_len = sizeof(suback3)},

	{.test_name = "PINGREQ",
	 .ctx = NULL, .eval_fcn = eval_msg_pingreq,
	 .expected = pingreq1, .expected_len = sizeof(pingreq1)},

	{.test_name = "PUBACK",
	 .ctx = &msg_puback1, .eval_fcn = eval_msg_puback,
	 .expected = puback1, .expected_len = sizeof(puback1)},

	{.test_name = "PUBREC",
	 .ctx = &msg_pubrec1, .eval_fcn = eval_msg_pubrec,
	 .expected = pubrec1, .expected_len = sizeof(pubrec1)},

	{.test_name = "PUBREL",
	 .ctx = &msg_pubrel1, .eval_fcn = eval_msg_pubrel,
	 .expected = pubrel1, .expected_len = sizeof(pubrel1)},

	{.test_name = "PUBCOMP",
	 .ctx = &msg_pubcomp1, .eval_fcn = eval_msg_pubcomp,
	 .expected = pubcomp1, .expected_len = sizeof(pubcomp1)},

	{.test_name = "UNSUBACK",
	 .ctx = &msg_unsuback1, .eval_fcn = eval_msg_unsuback,
	 .expected = unsuback1, .expected_len = sizeof(unsuback1)},

	/* last test case, do not remove it */
	{.test_name = NULL}
};

static void print_array(const u8_t *a, u16_t size)
{
	u16_t i;

	TC_PRINT("\n");
	for (i = 0U; i < size; i++) {
		TC_PRINT("%x ", a[i]);
		if ((i+1) % 8 == 0U) {
			TC_PRINT("\n");
		}
	}
	TC_PRINT("\n");
}

static
int eval_buffers(const struct buf_ctx *buf, const u8_t *expected, u16_t len)
{
	if (buf->end - buf->cur != len) {
		goto exit_eval;
	}

	if (memcmp(expected, buf->cur, buf->end - buf->cur) != 0) {
		goto exit_eval;
	}

	return TC_PASS;

exit_eval:
	TC_PRINT("FAIL\n");
	TC_PRINT("Computed:");
	print_array(buf->cur, buf->end - buf->cur);
	TC_PRINT("Expected:");
	print_array(expected, len);

	return TC_FAIL;
}

static int eval_msg_connect(struct mqtt_test *mqtt_test)
{
	struct mqtt_client *test_client;
	int rc;
	struct buf_ctx buf;

	test_client = (struct mqtt_client *)mqtt_test->ctx;

	client.clean_session = test_client->clean_session;
	client.client_id = test_client->client_id;
	client.will_topic = test_client->will_topic;
	client.will_retain = test_client->will_retain;
	client.will_message = test_client->will_message;
	client.user_name = test_client->user_name;
	client.password = test_client->password;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = connect_request_encode(&client, &buf);

	/**TESTPOINTS: Check connect_request_encode functions*/
	zassert_false(rc, "connect_request_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	return TC_PASS;
}

static int eval_msg_disconnect(struct mqtt_test *mqtt_test)
{
	int rc;
	struct buf_ctx buf;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = disconnect_encode(&buf);

	/**TESTPOINTS: Check disconnect_encode functions*/
	zassert_false(rc, "disconnect_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	return TC_PASS;
}

static int eval_msg_publish(struct mqtt_test *mqtt_test)
{
	struct mqtt_publish_param *param =
			(struct mqtt_publish_param *)mqtt_test->ctx;
	struct mqtt_publish_param dec_param;
	int rc;
	u8_t type_and_flags;
	u32_t length;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = publish_encode(param, &buf);

	/* Payload is not copied, copy it manually just after the header.*/
	memcpy(buf.end, param->message.payload.data,
	       param->message.payload.len);
	buf.end += param->message.payload.len;

	/**TESTPOINT: Check publish_encode function*/
	zassert_false(rc, "publish_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = publish_decode(type_and_flags, length, &buf, &dec_param);

	/**TESTPOINT: Check publish_decode function*/
	zassert_false(rc, "publish_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "message_id error");
	zassert_equal(dec_param.dup_flag, param->dup_flag,
		      "dup flag error");
	zassert_equal(dec_param.retain_flag, param->retain_flag,
		      "retain flag error");
	zassert_equal(dec_param.message.topic.qos, param->message.topic.qos,
		      "topic qos error");
	zassert_equal(dec_param.message.topic.topic.size,
		      param->message.topic.topic.size,
		      "topic len error");
	if (memcmp(dec_param.message.topic.topic.utf8,
		   param->message.topic.topic.utf8,
		   dec_param.message.topic.topic.size) != 0) {
		zassert_unreachable("topic content error");
	}
	zassert_equal(dec_param.message.payload.len,
		      param->message.payload.len,
		      "payload len error");

	return TC_PASS;
}

static int eval_msg_subscribe(struct mqtt_test *mqtt_test)
{
	struct mqtt_subscription_list *param =
				(struct mqtt_subscription_list *)mqtt_test->ctx;
	int rc;
	struct buf_ctx buf;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = subscribe_encode(param, &buf);

	/**TESTPOINT: Check subscribe_encode function*/
	zassert_false(rc, "subscribe_encode failed");

	return eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);
}

static int eval_msg_suback(struct mqtt_test *mqtt_test)
{
	struct mqtt_suback_param *param =
			(struct mqtt_suback_param *)mqtt_test->ctx;
	struct mqtt_suback_param dec_param;

	int rc;
	u8_t type_and_flags;
	u32_t length;
	struct buf_ctx buf;

	buf.cur = mqtt_test->expected;
	buf.end = mqtt_test->expected + mqtt_test->expected_len;

	memset(&dec_param, 0, sizeof(dec_param));

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = subscribe_ack_decode(&buf, &dec_param);

	/**TESTPOINT: Check subscribe_ack_decode function*/
	zassert_false(rc, "subscribe_ack_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");
	zassert_equal(dec_param.return_codes.len,
		      param->return_codes.len,
		      "topic count error");
	if (memcmp(dec_param.return_codes.data, param->return_codes.data,
		   dec_param.return_codes.len) != 0) {
		zassert_unreachable("subscribe result error");
	}

	return TC_PASS;
}

static int eval_msg_pingreq(struct mqtt_test *mqtt_test)
{
	int rc;
	struct buf_ctx buf;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = ping_request_encode(&buf);

	/**TESTPOINTS: Check eval_msg_pingreq functions*/
	zassert_false(rc, "ping_request_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	return TC_PASS;
}

static int eval_msg_puback(struct mqtt_test *mqtt_test)
{
	struct mqtt_puback_param *param =
			(struct mqtt_puback_param *)mqtt_test->ctx;
	struct mqtt_puback_param dec_param;
	int rc;
	u8_t type_and_flags;
	u32_t length;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = publish_ack_encode(param, &buf);

	/**TESTPOINTS: Check publish_ack_encode functions*/
	zassert_false(rc, "publish_ack_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = publish_ack_decode(&buf, &dec_param);

	zassert_false(rc, "publish_ack_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");

	return TC_PASS;
}

static int eval_msg_pubcomp(struct mqtt_test *mqtt_test)
{
	struct mqtt_pubcomp_param *param =
			(struct mqtt_pubcomp_param *)mqtt_test->ctx;
	struct mqtt_pubcomp_param dec_param;
	int rc;
	u32_t length;
	u8_t type_and_flags;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = publish_complete_encode(param, &buf);

	/**TESTPOINTS: Check publish_complete_encode functions*/
	zassert_false(rc, "publish_complete_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = publish_complete_decode(&buf, &dec_param);

	zassert_false(rc, "publish_complete_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");

	return TC_PASS;
}

static int eval_msg_pubrec(struct mqtt_test *mqtt_test)
{
	struct mqtt_pubrec_param *param =
			(struct mqtt_pubrec_param *)mqtt_test->ctx;
	struct mqtt_pubrec_param dec_param;
	int rc;
	u32_t length;
	u8_t type_and_flags;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = publish_receive_encode(param, &buf);

	/**TESTPOINTS: Check publish_receive_encode functions*/
	zassert_false(rc, "publish_receive_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = publish_receive_decode(&buf, &dec_param);

	zassert_false(rc, "publish_receive_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");

	return TC_PASS;
}

static int eval_msg_pubrel(struct mqtt_test *mqtt_test)
{
	struct mqtt_pubrel_param *param =
			(struct mqtt_pubrel_param *)mqtt_test->ctx;
	struct mqtt_pubrel_param dec_param;
	int rc;
	u32_t length;
	u8_t type_and_flags;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	rc = publish_release_encode(param, &buf);

	/**TESTPOINTS: Check publish_release_encode functions*/
	zassert_false(rc, "publish_release_encode failed");

	rc = eval_buffers(&buf, mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = publish_release_decode(&buf, &dec_param);

	zassert_false(rc, "publish_release_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");

	return TC_PASS;
}

static int eval_msg_unsuback(struct mqtt_test *mqtt_test)
{
	struct mqtt_unsuback_param *param =
			(struct mqtt_unsuback_param *)mqtt_test->ctx;
	struct mqtt_unsuback_param dec_param;
	int rc;
	u32_t length;
	u8_t type_and_flags;
	struct buf_ctx buf;

	memset(&dec_param, 0, sizeof(dec_param));

	buf.cur = mqtt_test->expected;
	buf.end = mqtt_test->expected + mqtt_test->expected_len;

	rc = fixed_header_decode(&buf, &type_and_flags, &length);

	zassert_false(rc, "fixed_header_decode failed");

	rc = unsubscribe_ack_decode(&buf, &dec_param);

	zassert_false(rc, "unsubscribe_ack_decode failed");

	zassert_equal(dec_param.message_id, param->message_id,
		      "packet identifier error");

	return TC_PASS;
}

void test_mqtt_packet(void)
{
	TC_START("MQTT Library test");

	int rc;
	int i;

	mqtt_client_init(&client);
	client.protocol_version = MQTT_VERSION_3_1_1;
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

	i = 0;
	do {
		struct mqtt_test *test = &mqtt_tests[i];

		if (test->test_name == NULL) {
			break;
		}

		rc = test->eval_fcn(test);
		TC_PRINT("[%s] %d - %s\n", TC_RESULT_TO_STR(rc), i + 1,
			test->test_name);

		/**TESTPOINT: Check eval_fcn*/
		zassert_false(rc, "mqtt_packet test error");

		i++;
	} while (1);

	mqtt_abort(&client);
}

void test_main(void)
{
	ztest_test_suite(test_mqtt_packet_fn,
		ztest_user_unit_test(test_mqtt_packet));
	ztest_run_test_suite(test_mqtt_packet_fn);
}
