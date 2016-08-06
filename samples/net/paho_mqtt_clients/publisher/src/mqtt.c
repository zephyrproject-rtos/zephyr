/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mqtt.h"

#include <errno.h>

int mqtt_cb_publish(struct mqtt_app_ctx_t *app,
		    int (*cb_publish)(struct mqtt_msg_t *msg))
{
	app->cb_publish = cb_publish;

	return 0;
}

int mqtt_init(struct mqtt_app_ctx_t *app)
{
	nano_sem_init(&app->sem);
	nano_sem_give(&app->sem);

	return 0;
}

int mqtt_configure(struct mqtt_app_ctx_t *app, const char *client_id,
	      int clean_session, enum mqtt_protocol proto)
{
	app->client->client_id = client_id;
	app->client->clean_session = clean_session;
	app->client->proto = proto;

	return 0;
}

int mqtt_auth(struct mqtt_app_ctx_t *app, const char *username,
	      const char *pass)
{
	app->client->username = username;
	app->client->pass = pass;

	return 0;
}

int mqtt_will(struct mqtt_app_ctx_t *app, const char *topic,
	      const char *msg, enum mqtt_qos qos, int retained)
{
	app->client->will_enabled = 1;
	app->client->will_topic = topic;
	app->client->will_payload = msg;
	app->client->will_qos = qos;
	app->client->will_retained = retained;

	return 0;
}

int mqtt_buffers(struct mqtt_app_ctx_t *app, struct app_buf_t *tx_buf,
		 struct app_buf_t *rx_buf)
{
	app->tx_buf = tx_buf;
	app->rx_buf = rx_buf;

	return 0;
}

int mqtt_network(struct mqtt_app_ctx_t *app, struct netz_ctx_t *netz_ctx)
{
	app->netz_ctx = netz_ctx;

	return 0;
}

int mqtt_connect(struct mqtt_app_ctx_t *app)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	int session;
	int conn_ack;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;
	rx_buf = app->rx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	rc = mqtt_pack_connect(tx_buf, app->client);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_connect;
	}

	rc = netz_tcp(netz_ctx);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tcp;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tx;
	}

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_rx;
	}

	rc = mqtt_unpack_connack(rx_buf, &session, &conn_ack);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_unpack_connack;
	}

	rc = -EINVAL;
	if (app->client->clean_session == 1) {
		if (session == 0 && conn_ack == 0) {
			rc = 0;
		}
	}
	/*
	 * TODO: validate CleanSession = 0
	 */

error_mqtt_pack_connect:
error_netz_tcp:
error_netz_tx:
error_netz_rx:
error_mqtt_unpack_connack:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_disconnect(struct mqtt_app_ctx_t *app)
{
	int rc;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	rc = mqtt_pack_disconnect(app->tx_buf);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_disconnect;
	}

	rc = netz_tx(app->netz_ctx, app->tx_buf);
	if (rc != 0) {
		rc = -EIO;
	}

error_mqtt_pack_disconnect:
	nano_sem_give(&app->sem);
	return rc;
}


static inline int mqtt_publish_qos1(struct mqtt_app_ctx_t *app, uint16_t id)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *rx_buf;
	uint8_t pkt_type;
	uint8_t dup = 0;
	uint16_t rcv_pkt_id = 0;
	int rc;

	netz_ctx = app->netz_ctx;
	rx_buf = app->rx_buf;

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = mqtt_unpack_ack(rx_buf, &pkt_type, &dup, &rcv_pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (pkt_type != MQTT_PUBACK) {
		return -EINVAL;
	}

	if (rcv_pkt_id != id) {
		return -EINVAL;
	}

	return 0;
}

static inline int mqtt_publish_qos2(struct mqtt_app_ctx_t *app,
				    const uint16_t pkt_id)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	uint8_t pkt_type;
	uint8_t dup = 0;
	uint16_t rcv_pkt_id;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;
	rx_buf = app->rx_buf;

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = mqtt_unpack_ack(rx_buf, &pkt_type, &dup, &rcv_pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (pkt_type != MQTT_PUBREC) {
		return -EINVAL;
	}
	if (rcv_pkt_id != pkt_id) {
		return -EINVAL;
	}

	rc = mqtt_pack_pubrel(tx_buf, dup, pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = mqtt_unpack_ack(rx_buf, &pkt_type, &dup, &rcv_pkt_id);
	if (rc != 0) {
		return -EINVAL;
	}

	if (pkt_type != MQTT_PUBCOMP) {
		return -EINVAL;
	}
	if (rcv_pkt_id != pkt_id) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_publish(struct mqtt_app_ctx_t *app, struct mqtt_msg_t *msg,
		 enum mqtt_qos qos, int retained)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;

	msg->dup = 0;
	msg->qos = qos;
	msg->retained = retained;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	if (msg->qos != MQTT_QoS0) {
		mqtt_next_pktid(app->client, &msg->pkt_id);
	} else {
		msg->pkt_id = 0;
	}

	rc = mqtt_pack_publish(tx_buf, msg);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_publish;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tx;
	}

	switch (msg->qos) {
	case MQTT_QoS0:
		break;

	case MQTT_QoS1:
		rc = mqtt_publish_qos1(app, msg->pkt_id);
		if (rc != 0) {
			rc = -EINVAL;
		}
		break;

	case MQTT_QoS2:
		rc = mqtt_publish_qos2(app, msg->pkt_id);
		if (rc != 0) {
			rc = -EINVAL;
		}
		break;

	default:
		rc = -EINVAL;
	}

error_mqtt_pack_publish:
error_netz_tx:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_pingreq(struct mqtt_app_ctx_t *app)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	uint8_t pkt_type;
	uint8_t dup = 0;
	uint16_t rcv_pkt_id;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;
	rx_buf = app->rx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	rc = mqtt_pack_pingreq(tx_buf);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_pingreq;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tx;
	}

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_rx;
	}

	rc = mqtt_unpack_ack(rx_buf, &pkt_type, &dup, &rcv_pkt_id);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_unpack_ack;
	}

	if (pkt_type != MQTT_PINGRESP) {
		rc = -EINVAL;
	}

error_mqtt_pack_pingreq:
error_netz_tx:
error_netz_rx:
error_mqtt_unpack_ack:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_subscribe(struct mqtt_app_ctx_t *app,
		   char *topic, enum mqtt_qos qos)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	uint16_t pkt_id;
	uint16_t rcv_pkt_id;
	int granted_qos;
	int dup = 0;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;
	rx_buf = app->rx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	mqtt_next_pktid(app->client, &pkt_id);

	rc = mqtt_pack_subscribe(tx_buf, dup, pkt_id, topic, qos);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_subscribe;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tx;
	}

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_rx;
	}

	rc = mqtt_unpack_suback(rx_buf, &rcv_pkt_id, &granted_qos);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_unpack_suback;
	}

	if (rcv_pkt_id != pkt_id) {
		rc = -EINVAL;
	} else {
		rc = granted_qos;
	}

error_mqtt_pack_subscribe:
error_netz_tx:
error_netz_rx:
error_mqtt_unpack_suback:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_unsubscribe(struct mqtt_app_ctx_t *app, char *topic)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	uint16_t pkt_id;
	uint16_t rcv_pkt_id;
	int dup = 0;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;
	rx_buf = app->rx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	mqtt_next_pktid(app->client, &pkt_id);

	rc = mqtt_pack_unsubscribe(tx_buf, dup, pkt_id, topic);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_unsubscribe;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_tx;
	}

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_rx;
	}

	rc = mqtt_unpack_unsuback(rx_buf, &rcv_pkt_id);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_unpack_unsuback;
	}

	if (rcv_pkt_id != pkt_id) {
		rc = -EINVAL;
	}

error_mqtt_pack_unsubscribe:
error_netz_tx:
error_netz_rx:
error_mqtt_unpack_unsuback:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_handle_publish(struct mqtt_app_ctx_t *app)
{
	struct mqtt_msg_t msg;
	uint16_t rcv_pkt_id;
	int8_t rcv_pkt_type;
	uint8_t dup;
	int rc;

	rc = mqtt_unpack_publish(app->rx_buf, &msg);
	if (rc != 0) {
		return -EINVAL;
	}

	switch (msg.qos) {
	case MQTT_QoS0:
		break;

	case MQTT_QoS1:
		rc = mqtt_pack_msg(app->tx_buf, MQTT_PUBACK, msg.pkt_id, 0);
		if (rc != 0) {
			return -EINVAL;
		}

		rc = netz_tx(app->netz_ctx, app->tx_buf);
		if (rc != 0) {
			return -EIO;
		}

		break;

	case MQTT_QoS2:
		rc = mqtt_pack_msg(app->tx_buf, MQTT_PUBREC, msg.pkt_id, 0);
		if (rc != 0) {
			return -EINVAL;
		}

		rc = netz_tx(app->netz_ctx, app->tx_buf);
		if (rc != 0) {
			return -EIO;
		}

		rc = netz_rx(app->netz_ctx, app->rx_buf);
		if (rc != 0) {
			return -EIO;
		}

		rc = mqtt_unpack_ack(app->rx_buf, &rcv_pkt_type, &dup,
				     &rcv_pkt_id);
		if (rc != 0) {
			return -EINVAL;
		}

		if (rcv_pkt_type != MQTT_PUBREL) {
			return -EINVAL;
		}

		if (msg.pkt_id != rcv_pkt_id) {
			return -EINVAL;
		}

		rc = mqtt_pack_msg(app->tx_buf, MQTT_PUBCOMP, msg.pkt_id, 0);
		if (rc != 0) {
			return -EINVAL;
		}

		rc = netz_tx(app->netz_ctx, app->tx_buf);
		if (rc != 0) {
			return -EIO;
		}

		break;

	default:
		return -EINVAL;
	}

	if (app->cb_publish != NULL) {
		return app->cb_publish(&msg);
	}

	return 0;
}

int mqtt_handle_pingreq(struct mqtt_app_ctx_t *app)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	int rc;

	netz_ctx = app->netz_ctx;
	tx_buf = app->tx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	rc = mqtt_pack_pingresp(tx_buf);
	if (rc != 0) {
		rc = -EINVAL;
		goto error_mqtt_pack_pingresp;
	}

	rc = netz_tx(netz_ctx, tx_buf);
	if (rc != 0) {
		rc = -EIO;
	}

error_mqtt_pack_pingresp:
	nano_sem_give(&app->sem);
	return rc;
}

int mqtt_read(struct mqtt_app_ctx_t *app)
{
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *rx_buf;
	int pkt_type;
	int rc;

	netz_ctx = app->netz_ctx;
	rx_buf = app->rx_buf;

	nano_sem_take(&app->sem, TICKS_UNLIMITED);

	rc = netz_rx(netz_ctx, rx_buf);
	if (rc != 0) {
		rc = -EIO;
		goto error_netz_rx;
	}

	if (rx_buf->length < 2) {
		rc = -ENOMEM;
		goto error_nomem;
	}

	pkt_type = (rx_buf->buf[0] & 0xF0) >> 4;

	/* This switch-case will be used for packet-handling routines
	 * that will be coded in future releases.
	 */
	switch (pkt_type) {
	case MQTT_PUBLISH:
		rc = mqtt_handle_publish(app);
		break;
	case MQTT_CONNECT:
	case MQTT_CONNACK:
	case MQTT_PUBACK:
	case MQTT_PUBREC:
	case MQTT_PUBREL:
	case MQTT_PUBCOMP:
	case MQTT_SUBSCRIBE:
	case MQTT_SUBACK:
	case MQTT_UNSUBSCRIBE:
	case MQTT_UNSUBACK:
		break;
	case MQTT_PINGREQ:
		rc = mqtt_handle_pingreq(app);
		break;
	case MQTT_PINGRESP:
		break;
	case MQTT_DISCONNECT:
		break;
	default:
		rc = -EINVAL;
		break;
	}

error_netz_rx:
error_nomem:
	nano_sem_give(&app->sem);
	return rc;
}
