/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <mqtt_pkt.h>
#include <misc/util.h>	/* for ARRAY_SIZE */
#include <ztest.h>

#define RC_STR(rc)	(rc == TC_PASS ? PASS : FAIL)

#define CLIENTID	"zephyr"
#define CLIENTID_LEN	6
#define TOPIC		"sensors"
#define TOPIC_LEN	7
#define WILL_TOPIC	"quitting"
#define WILL_TOPIC_LEN	8
#define USERNAME	"zephyr1"
#define USERNAME_LEN	7

/* MQTT messages in this test are under 256 bytes */
#define BUF_SIZE	256
static u8_t buf[BUF_SIZE];
static u16_t buf_len;

/**
 * @brief MQTT test structure
 */
struct mqtt_test {
	/* test name, for example: "test connect 1" */
	const char *test_name;

	/* cast to something like:
	 * struct mqtt_connect_msg *msg_connect = (struct mqtt_connect_msg *)msg
	 */
	void *msg;

	/* pointer to the eval routine, for example:
	 * eval_fcn = eval_msg_connect
	 */
	int (*eval_fcn)(struct mqtt_test *);

	/* expected result */
	u8_t *expected;

	/* length of 'expected' */
	u16_t expected_len;
};

#define MAX_TOPICS	4

/**
 * @brief MQTT SUBSCRIBE msg
 */
struct msg_subscribe {
	u16_t pkt_id;
	int items;
	const char *topics[MAX_TOPICS];
	enum mqtt_qos qos[MAX_TOPICS];
};

/**
 * @brief MQTT SUBACK msg
 */
struct msg_suback {
	u16_t pkt_id;
	int elements;
	enum mqtt_qos qos[MAX_TOPICS];
};

/**
 * @brief MQTT msg with Packet Identifier as payload:
 *	  PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK
 */
struct msg_pkt_id {
	u16_t pkt_id;
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
 * @brief eval_msg_pingresp	Evaluate the given mqtt_test against the
 *				pingresp packing/unpacking routines.
 * @param [in] mqtt_test	MQTT test structure
 * @return			TC_PASS on success
 * @return			TC_FAIL on error
 */
static int eval_msg_pingresp(struct mqtt_test *mqtt_test);

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
 * @param [in] buf_len		'buf' length
 * @param [in] expected		Expected buffer
 * @param [in] len		'expected' len
 * @return			TC_PASS on success
 * @return			TC_FAIL on error and prints both buffers
 */
static int eval_buffers(u8_t *buf, u16_t buf_len,
			u8_t *expected, u16_t len);

/**
 * @brief print_array		Prints the array 'a' of 'size' elements
 * @param a			The array
 * @param size			Array's size
 */
static void print_array(u8_t *a, u16_t size);

/**
 * @brief test_strlen		Computes the C string length allowing NULL as
 *				input parameter
 * @param [in] str		Input string
 * @return			String length, assuming that strlen(NULL) is 0
 */
static size_t test_strlen(const char *str);

/*
 * MQTT CONNECT msg:
 * Clean session: 1	Client id: [6] 'zephyr'	Will flag: 0
 * Will QoS: 0		Will retain: 0		Will topic: [0]
 * Will msg: [0]	Keep alive: 0		User name: [0]
 * Password: [0]
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors
 */
static
u8_t connect1[] = {0x10, 0x12, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x02, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72};

static struct mqtt_connect_msg msg_connect1 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 0,
	.will_qos = 0, .will_retain = 0, .will_topic = NULL,
	.will_msg = NULL, .will_msg_len = 0,
	.keep_alive = 0, .user_name = NULL,
	.password = NULL, .password_len = 0
};

/*
 * MQTT CONNECT msg:
 * Same message as connect1, but change the keep alive parameter
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 365 -t sensors
 */
static
u8_t connect2[] = {0x10, 0x12, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x02, 0x01, 0x6d, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72};

static struct mqtt_connect_msg msg_connect2 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 0,
	.will_qos = 0, .will_retain = 0, .will_topic = NULL,
	.will_msg = NULL, .will_msg_len = 0,
	.keep_alive = 365, .user_name = NULL,
	.password = NULL, .password_len = 0
};

/*
 * MQTT CONNECT msg:
 * Clean session: 1	Client id: [6] 'zephyr'	Will flag: 1
 * Will QoS: 0		Will retain: 0		Will topic: [8] quitting
 * Will msg: [3] bye	Keep alive: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors --will-topic quitting \
 * --will-qos 0 --will-payload bye
 */
static
u8_t connect3[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x06, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		      0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		      0x62, 0x79, 0x65};

static struct mqtt_connect_msg msg_connect3 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 1,
	.will_qos = 0, .will_retain = 0, .will_topic = WILL_TOPIC,
	.will_topic_len = WILL_TOPIC_LEN,
	.will_msg = (u8_t *)"bye",
	.will_msg_len = 3,
	.keep_alive = 0, .user_name = NULL,
	.password = NULL, .password_len = 0
};

/*
 * MQTT CONNECT msg:
 * Same message as connect3, but set  Will retain: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors --will-topic quitting \
 * --will-qos 0 --will-payload bye  --will-retain
 */
static
u8_t connect4[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x26, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		      0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		      0x62, 0x79, 0x65};

static struct mqtt_connect_msg msg_connect4 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 1,
	.will_qos = 0, .will_retain = 1, .will_topic = WILL_TOPIC,
	.will_topic_len = WILL_TOPIC_LEN,
	.will_msg = (u8_t *)"bye",
	.will_msg_len = 3,
	.keep_alive = 0, .user_name = NULL,
	.password = NULL, .password_len = 0
};

/*
 * MQTT CONNECT msg:
 * Same message as connect3, but set  Will QoS: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye
 */
static
u8_t connect5[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x0e, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		      0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		      0x62, 0x79, 0x65};

static struct mqtt_connect_msg msg_connect5 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 1,
	.will_qos = 1, .will_retain = 0, .will_topic = WILL_TOPIC,
	.will_topic_len = WILL_TOPIC_LEN,
	.will_msg = (u8_t *)"bye",
	.will_msg_len = 3,
	.keep_alive = 0, .user_name = NULL,
	.password = NULL, .password_len = 0
};

/*
 * MQTT CONNECT msg:
 * Same message as connect5, but set Will retain: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye --will-retain
 */
static
u8_t connect6[] = {0x10, 0x21, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0x2e, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		      0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		      0x62, 0x79, 0x65};

static struct mqtt_connect_msg msg_connect6 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 1,
	.will_qos = 1, .will_retain = 1, .will_topic = WILL_TOPIC,
	.will_topic_len = WILL_TOPIC_LEN,
	.will_msg = (u8_t *)"bye",
	.will_msg_len = 3,
	.keep_alive = 0, .user_name = NULL,
	.password = NULL, .password_len = 0
};


/*
 * MQTT CONNECT msg:
 * Same message as connect6, but set username: zephyr1 and password: password
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -k 0 -t sensors --will-topic quitting \
 * --will-qos 1 --will-payload bye --will-retain -u zephyr1 -P password
 */
static
u8_t connect7[] = {0x10, 0x34, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
		      0x04, 0xee, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65,
		      0x70, 0x68, 0x79, 0x72, 0x00, 0x08, 0x71, 0x75,
		      0x69, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x03,
		      0x62, 0x79, 0x65, 0x00, 0x07, 0x7a, 0x65, 0x70,
		      0x68, 0x79, 0x72, 0x31, 0x00, 0x08, 0x70, 0x61,
		      0x73, 0x73, 0x77, 0x6f, 0x72, 0x64};

static struct mqtt_connect_msg msg_connect7 = {
	.clean_session = 1, .client_id = CLIENTID,
	.client_id_len = CLIENTID_LEN,
	.will_flag = 1,
	.will_qos = 1, .will_retain = 1, .will_topic = WILL_TOPIC,
	.will_topic_len = WILL_TOPIC_LEN,
	.will_msg = (u8_t *)"bye",
	.will_msg_len = 3,
	.keep_alive = 0, .user_name = USERNAME,
	.user_name_len = USERNAME_LEN,
	.password = (u8_t *)"password",
	.password_len = 8
};

static
u8_t disconnect1[] = {0xe0, 0x00};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 0, Retain: 0, topic: sensors, message: OK
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 0 -m "OK"
 */
static
u8_t publish1[] = {0x30, 0x0b, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		      0x6f, 0x72, 0x73, 0x4f, 0x4b};

static struct mqtt_publish_msg msg_publish1 = {
	.dup = 0, .qos = 0, .retain = 0, .topic = TOPIC,
	.topic_len = TOPIC_LEN,
	.pkt_id = 0,
	.msg = (u8_t *)"OK",
	.msg_len = 2,
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 0, Retain: 1, topic: sensors, message: OK
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 0 -m "OK" -r
 */
static
u8_t publish2[] = {0x31, 0x0b, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		      0x6f, 0x72, 0x73, 0x4f, 0x4b};

static struct mqtt_publish_msg msg_publish2 = {
	.dup = 0, .qos = 0, .retain = 1, .topic = TOPIC,
	.topic_len = TOPIC_LEN,
	.pkt_id = 0,
	.msg = (u8_t *)"OK",
	.msg_len = 2
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 1, Retain: 1, topic: sensors, message: OK, pkt_id: 1
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 1 -m "OK" -r
 */
static
u8_t publish3[] = {0x33, 0x0d, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		      0x6f, 0x72, 0x73, 0x00, 0x01, 0x4f, 0x4b};

static struct mqtt_publish_msg msg_publish3 = {
	.dup = 0, .qos = 1, .retain = 1, .topic = TOPIC,
	.topic_len = TOPIC_LEN,
	.pkt_id = 1,
	.msg = (u8_t *)"OK",
	.msg_len = 2
};

/*
 * MQTT PUBLISH msg:
 * DUP: 0, QoS: 2, Retain: 0, topic: sensors, message: OK, pkt_id: 1
 *
 * Message can be generated by the following command:
 * mosquitto_pub -V mqttv311 -i zephyr -t sensors -q 2 -m "OK"
 */
static
u8_t publish4[] = {0x34, 0x0d, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
		      0x6f, 0x72, 0x73, 0x00, 0x01, 0x4f, 0x4b};
static struct mqtt_publish_msg msg_publish4 = {
	.dup = 0, .qos = 2, .retain = 0, .topic = TOPIC,
	.topic_len = TOPIC_LEN,
	.pkt_id = 1,
	.msg = (u8_t *)"OK",
	.msg_len = 2
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 0
 */
static
u8_t subscribe1[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
			0x6e, 0x73, 0x6f, 0x72, 0x73, 0x00};
static struct msg_subscribe msg_subscribe1 = {
	 .pkt_id = 1, .items = 1,
	 .topics = {"sensors"}, .qos = {MQTT_QoS0}
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 1
 */
static
u8_t subscribe2[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
			0x6e, 0x73, 0x6f, 0x72, 0x73, 0x01};
static struct msg_subscribe msg_subscribe2 = {
	 .pkt_id = 1, .items = 1,
	 .topics = {"sensors"}, .qos = {MQTT_QoS1}
};

/*
 * MQTT SUBSCRIBE msg:
 * pkt_id: 1, topic: sensors, qos: 2
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 2
 */
static
u8_t subscribe3[] = {0x82, 0x0c, 0x00, 0x01, 0x00, 0x07, 0x73, 0x65,
			0x6e, 0x73, 0x6f, 0x72, 0x73, 0x02};
static struct msg_subscribe msg_subscribe3 = {
	 .pkt_id = 1, .items = 1,
	 .topics = {"sensors"}, .qos = {MQTT_QoS2}
};

/*
 * MQTT SUBACK msg
 * pkt_id: 1, qos: 0
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 0
 */
static
u8_t suback1[] = {0x90, 0x03, 0x00, 0x01, 0x00};
static struct msg_suback msg_suback1 = {
	.pkt_id = 1, .elements = 1, .qos = {MQTT_QoS0}
};

/*
 * MQTT SUBACK message
 * pkt_id: 1, qos: 1
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 1
 */
static
u8_t suback2[] = {0x90, 0x03, 0x00, 0x01, 0x01};
static struct msg_suback msg_suback2 = {
	.pkt_id = 1, .elements = 1, .qos = {MQTT_QoS1}
};

/*
 * MQTT SUBACK message
 * pkt_id: 1, qos: 2
 *
 * Message can be generated by the following command:
 * mosquitto_sub -V mqttv311 -i zephyr -t sensors -q 2
 */
static
u8_t suback3[] = {0x90, 0x03, 0x00, 0x01, 0x02};
static struct msg_suback msg_suback3 = {
	.pkt_id = 1, .elements = 1, .qos = {MQTT_QoS2}
};

static
u8_t pingreq1[] = {0xc0, 0x00};

static
u8_t pingresp1[] = {0xd0, 0x00};

static
u8_t puback1[] = {0x40, 0x02, 0x00, 0x01};
static struct msg_pkt_id msg_puback1 = {.pkt_id = 1};

static
u8_t pubrec1[] = {0x50, 0x02, 0x00, 0x01};
static struct msg_pkt_id msg_pubrec1 = {.pkt_id = 1};

static
u8_t pubrel1[] = {0x62, 0x02, 0x00, 0x01};
static struct msg_pkt_id msg_pubrel1 = {.pkt_id = 1};

static
u8_t pubcomp1[] = {0x70, 0x02, 0x00, 0x01};
static struct msg_pkt_id msg_pubcomp1 = {.pkt_id = 1};

static
u8_t unsuback1[] = {0xb0, 0x02, 0x00, 0x01};
static struct msg_pkt_id msg_unsuback1 = {.pkt_id = 1};

static
struct mqtt_test mqtt_tests[] = {
	{.test_name = "CONNECT, new session, zeros",
	 .msg = &msg_connect1, .eval_fcn = eval_msg_connect,
	 .expected = connect1, .expected_len = sizeof(connect1)},

	{.test_name = "CONNECT, new session, zeros and keep alive = 365",
	 .msg = &msg_connect2, .eval_fcn = eval_msg_connect,
	 .expected = connect2, .expected_len = sizeof(connect2)},

	{.test_name = "CONNECT, new session, will",
	 .msg = &msg_connect3, .eval_fcn = eval_msg_connect,
	 .expected = connect3, .expected_len = sizeof(connect3)},

	{.test_name = "CONNECT, new session, will retain",
	 .msg = &msg_connect4, .eval_fcn = eval_msg_connect,
	 .expected = connect4, .expected_len = sizeof(connect4)},

	{.test_name = "CONNECT, new session, will qos = 1",
	 .msg = &msg_connect5, .eval_fcn = eval_msg_connect,
	 .expected = connect5, .expected_len = sizeof(connect5)},

	{.test_name = "CONNECT, new session, will qos = 1, will retain",
	 .msg = &msg_connect6, .eval_fcn = eval_msg_connect,
	 .expected = connect6, .expected_len = sizeof(connect6)},

	{.test_name = "CONNECT, new session, username and password",
	 .msg = &msg_connect7, .eval_fcn = eval_msg_connect,
	 .expected = connect7, .expected_len = sizeof(connect7)},

	{.test_name = "DISCONNECT",
	 .msg = NULL, .eval_fcn = eval_msg_disconnect,
	 .expected = disconnect1, .expected_len = sizeof(disconnect1)},

	{.test_name = "PUBLISH, qos = 0",
	 .msg = &msg_publish1, .eval_fcn = eval_msg_publish,
	 .expected = publish1, .expected_len = sizeof(publish1)},

	{.test_name = "PUBLISH, retain = 1",
	 .msg = &msg_publish2, .eval_fcn = eval_msg_publish,
	 .expected = publish2, .expected_len = sizeof(publish2)},

	{.test_name = "PUBLISH, retain = 1, qos = 1",
	 .msg = &msg_publish3, .eval_fcn = eval_msg_publish,
	 .expected = publish3, .expected_len = sizeof(publish3)},

	{.test_name = "PUBLISH, qos = 2",
	 .msg = &msg_publish4, .eval_fcn = eval_msg_publish,
	 .expected = publish4, .expected_len = sizeof(publish4)},

	{.test_name = "SUBSCRIBE, one topic, qos = 0",
	 .msg = &msg_subscribe1, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe1, .expected_len = sizeof(subscribe1)},

	{.test_name = "SUBSCRIBE, one topic, qos = 1",
	 .msg = &msg_subscribe2, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe2, .expected_len = sizeof(subscribe2)},

	{.test_name = "SUBSCRIBE, one topic, qos = 2",
	 .msg = &msg_subscribe3, .eval_fcn = eval_msg_subscribe,
	 .expected = subscribe3, .expected_len = sizeof(subscribe3)},

	{.test_name = "SUBACK, one topic, qos = 0",
	 .msg = &msg_suback1, .eval_fcn = eval_msg_suback,
	 .expected = suback1, .expected_len = sizeof(suback1)},

	{.test_name = "SUBACK, one topic, qos = 1",
	 .msg = &msg_suback2, .eval_fcn = eval_msg_suback,
	 .expected = suback2, .expected_len = sizeof(suback2)},

	{.test_name = "SUBACK, one topic, qos = 2",
	 .msg = &msg_suback3, .eval_fcn = eval_msg_suback,
	 .expected = suback3, .expected_len = sizeof(suback3)},

	{.test_name = "PINGREQ",
	 .msg = NULL, .eval_fcn = eval_msg_pingreq,
	 .expected = pingreq1, .expected_len = sizeof(pingreq1)},

	{.test_name = "PINGRESP",
	 .msg = NULL, .eval_fcn = eval_msg_pingresp,
	 .expected = pingresp1, .expected_len = sizeof(pingresp1)},

	{.test_name = "PUBACK",
	 .msg = &msg_puback1, .eval_fcn = eval_msg_puback,
	 .expected = puback1, .expected_len = sizeof(puback1)},

	{.test_name = "PUBREC",
	 .msg = &msg_pubrec1, .eval_fcn = eval_msg_pubrec,
	 .expected = pubrec1, .expected_len = sizeof(pubrec1)},

	{.test_name = "PUBREL",
	 .msg = &msg_pubrel1, .eval_fcn = eval_msg_pubrel,
	 .expected = pubrel1, .expected_len = sizeof(pubrel1)},

	{.test_name = "PUBCOMP",
	 .msg = &msg_pubcomp1, .eval_fcn = eval_msg_pubcomp,
	 .expected = pubcomp1, .expected_len = sizeof(pubcomp1)},

	{.test_name = "UNSUBACK",
	 .msg = &msg_unsuback1, .eval_fcn = eval_msg_unsuback,
	 .expected = unsuback1, .expected_len = sizeof(unsuback1)},

	/* last test case, do not remove it */
	{.test_name = NULL}
};

static void print_array(u8_t *a, u16_t size)
{
	u16_t i;

	TC_PRINT("\n");
	for (i = 0; i < size; i++) {
		TC_PRINT("%x ", a[i]);
		if ((i+1) % 8 == 0) {
			TC_PRINT("\n");
		}
	}
	TC_PRINT("\n");
}

static size_t test_strlen(const char *str)
{
	if (str == NULL) {
		return 0;
	}

	return strlen(str);
}

static
int eval_buffers(u8_t *buf, u16_t buf_len, u8_t *expected,
		 u16_t len)
{
	if (buf_len != len) {
		goto exit_eval;
	}

	if (memcmp(expected, buf, buf_len) != 0) {
		goto exit_eval;
	}

	return TC_PASS;

exit_eval:
	TC_PRINT("%s\n", FAIL);
	TC_PRINT("Computed:");
	print_array(buf, buf_len);
	TC_PRINT("Expected:");
	print_array(expected, len);

	return TC_FAIL;
}

static int eval_msg_connect(struct mqtt_test *mqtt_test)
{
	struct mqtt_connect_msg *msg;
	struct mqtt_connect_msg msg2;
	int rc;

	msg = (struct mqtt_connect_msg *)mqtt_test->msg;

	rc = mqtt_pack_connect(buf, &buf_len, sizeof(buf), msg);

	/**TESTPOINTS: Check eval_msg_connect functions*/
	zassert_false(rc, "mqtt_pack_connect failed");

	rc = eval_buffers(buf, buf_len,
			  mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = mqtt_unpack_connect(buf, buf_len, &msg2);

	zassert_false(rc, "mqtt_unpack_connect failed");

	zassert_equal(msg->clean_session, msg2.clean_session,
		"clean session check failed");

	zassert_equal(test_strlen(msg->client_id), msg2.client_id_len,
		"client ID length check failed");

	zassert_false(memcmp(msg->client_id, msg2.client_id,
				msg2.client_id_len),
				"client ID check failed");

	zassert_equal(msg->will_flag, msg2.will_flag,
		"will flag error");

	zassert_equal(msg->will_qos, msg2.will_qos,
		"will QoS error");

	zassert_equal(msg->will_retain, msg2.will_retain,
		"will retain error");

	zassert_equal(test_strlen(msg->will_topic), msg2.will_topic_len,
		"will topic length error");

	zassert_false(memcmp(msg->will_topic, msg2.will_topic,
				msg2.will_topic_len),
				"will topic error");

	zassert_equal(msg->will_msg_len, msg2.will_msg_len,
		"will msg len error");

	zassert_false(memcmp(msg->will_msg, msg2.will_msg, msg2.will_msg_len),
		"will msg error");

	zassert_equal(msg->keep_alive, msg2.keep_alive,
		"keep alive error");

	zassert_equal(test_strlen(msg->user_name), msg2.user_name_len,
		"username length error");

	zassert_false(memcmp(msg->user_name, msg2.user_name,
				msg2.user_name_len),
				"username error");

	zassert_equal(msg->password_len, msg2.password_len,
		"password length error");

	zassert_false(memcmp(msg->password, msg2.password, msg2.password_len),
		"password error");

	return TC_PASS;
}

static int eval_msg_disconnect(struct mqtt_test *mqtt_test)
{
	int rc;

	rc = mqtt_pack_disconnect(buf, &buf_len, sizeof(buf));

	/**TESTPOINTS: Check eval_msg_disconnect functions*/
	zassert_false(rc, "mqtt_pack_disconnect failed");

	rc = eval_buffers(buf, buf_len,
			  mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = mqtt_unpack_disconnect(buf, buf_len);

	zassert_false(rc, "mqtt_unpack_disconnect failed");

	return TC_PASS;
}

static int eval_msg_publish(struct mqtt_test *mqtt_test)
{
	struct mqtt_publish_msg *msg;
	int rc;

	msg = (struct mqtt_publish_msg *)mqtt_test->msg;

	rc = mqtt_pack_publish(buf, &buf_len, sizeof(buf), msg);

	/**TESTPOINT: Check eval_msg_publish function*/
	zassert_false(rc, "mqtt_pack_publish failed");

	return eval_buffers(buf, buf_len,
			    mqtt_test->expected, mqtt_test->expected_len);
}

static int eval_msg_subscribe(struct mqtt_test *mqtt_test)
{
	struct msg_subscribe *msg = (struct msg_subscribe *)mqtt_test->msg;
	int rc;

	rc = mqtt_pack_subscribe(buf, &buf_len, sizeof(buf), msg->pkt_id,
				 msg->items, msg->topics, msg->qos);

	/**TESTPOINT: Check eval_msg_subscribe function*/
	zassert_false(rc, "mqtt_pack_subscribe failed");

	return eval_buffers(buf, buf_len,
			    mqtt_test->expected, mqtt_test->expected_len);
}

static int eval_msg_suback(struct mqtt_test *mqtt_test)
{
	struct msg_suback *msg = (struct msg_suback *)mqtt_test->msg;
	int rc;

	rc = mqtt_pack_suback(buf, &buf_len, sizeof(buf),
			      msg->pkt_id, msg->elements, msg->qos);

	/**TESTPOINT: Check eval_msg_suback function*/
	zassert_false(rc, "mqtt_pack_suback failed");

	return eval_buffers(buf, buf_len,
			    mqtt_test->expected, mqtt_test->expected_len);
}

static int eval_msg_pingreq(struct mqtt_test *mqtt_test)
{
	int rc;

	rc = mqtt_pack_pingreq(buf, &buf_len, sizeof(buf));

	/**TESTPOINTS: Check eval_msg_pingreq functions*/
	zassert_false(rc, "mqtt_pack_pingreq failed");

	rc = eval_buffers(buf, buf_len,
			  mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = mqtt_unpack_pingreq(buf, buf_len);

	zassert_false(rc, "mqtt_unpack_pingreq failed");

	return TC_PASS;
}

static int eval_msg_pingresp(struct mqtt_test *mqtt_test)
{
	int rc;

	rc = mqtt_pack_pingresp(buf, &buf_len, sizeof(buf));

	/**TESTPOINTS: Check evam_msg_pingresp functions*/
	zassert_false(rc, "mqtt_pack_pingresp failed");

	rc = eval_buffers(buf, buf_len,
			  mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = mqtt_unpack_pingresp(buf, buf_len);

	zassert_false(rc, "mqtt_unpack_pingresp failed");

	return TC_PASS;
}

static
int eval_msg_packet_id(struct mqtt_test *mqtt_test, enum mqtt_packet type)
{
	int (*pack)(u8_t *, u16_t *, u16_t, u16_t) = NULL;
	int (*unpack)(u8_t *, u16_t, u16_t *) = NULL;
	struct msg_pkt_id *msg = (struct msg_pkt_id *)mqtt_test->msg;
	u16_t pkt_id;
	int rc;

	switch (type) {
	case MQTT_PUBACK:
		pack = mqtt_pack_puback;
		unpack = mqtt_unpack_puback;
		break;
	case MQTT_PUBCOMP:
		pack = mqtt_pack_pubcomp;
		unpack = mqtt_unpack_pubcomp;
		break;
	case MQTT_PUBREC:
		pack = mqtt_pack_pubrec;
		unpack = mqtt_unpack_pubrec;
		break;
	case MQTT_PUBREL:
		pack = mqtt_pack_pubrel;
		unpack = mqtt_unpack_pubrel;
		break;
	case MQTT_UNSUBACK:
		pack = mqtt_pack_unsuback;
		unpack = mqtt_unpack_unsuback;
		break;
	default:
		return TC_FAIL;
	}

	rc = pack(buf, &buf_len, sizeof(buf), msg->pkt_id);

	/**TESTPOINTS: Check eval_msg_packet_id functions*/
	zassert_false(rc, "pack failed");

	rc = eval_buffers(buf, buf_len,
			  mqtt_test->expected, mqtt_test->expected_len);

	zassert_false(rc, "eval_buffers failed");

	rc = unpack(buf, buf_len, &pkt_id);

	zassert_false(rc, "unpack failed");

	zassert_equal(pkt_id, msg->pkt_id, "packet identifier error");

	return TC_PASS;
}

static int eval_msg_puback(struct mqtt_test *mqtt_test)
{
	return eval_msg_packet_id(mqtt_test, MQTT_PUBACK);
}

static int eval_msg_pubcomp(struct mqtt_test *mqtt_test)
{
	return eval_msg_packet_id(mqtt_test, MQTT_PUBCOMP);
}

static int eval_msg_pubrec(struct mqtt_test *mqtt_test)
{
	return eval_msg_packet_id(mqtt_test, MQTT_PUBREC);
}

static int eval_msg_pubrel(struct mqtt_test *mqtt_test)
{
	return eval_msg_packet_id(mqtt_test, MQTT_PUBREL);
}

static int eval_msg_unsuback(struct mqtt_test *mqtt_test)
{
	return eval_msg_packet_id(mqtt_test, MQTT_UNSUBACK);
}

void run_tests(void)
{
	TC_START("MQTT Library test");

	int rc;
	int i;

	i = 0;
	do {
		struct mqtt_test *test = &mqtt_tests[i];

		if (test->test_name == NULL) {
			break;
		}

		rc = test->eval_fcn(test);
		TC_PRINT("[%s] %d - %s\n", RC_STR(rc), i + 1, test->test_name);

		/**TESTPOINT: Check eval_fcn*/
		zassert_false(rc, "mqtt_packet test error");

		i++;
	} while (1);
}


void test_main(void)
{
	ztest_test_suite(test_mqtt_packet_fn,
		ztest_unit_test(run_tests));
	ztest_run_test_suite(test_mqtt_packet_fn);
}
