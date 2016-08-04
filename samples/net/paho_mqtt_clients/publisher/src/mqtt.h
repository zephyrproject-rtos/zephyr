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

#ifndef _MQTT_H_
#define _MQTT_H_

#include "netz.h"
#include "mqtt_pack.h"

/**
 * @brief	The mqtt_app_ctx_t struct
 *
 * @details	This structure encapsulates  all the components required
 *		to write a MQTT application: client context, network
 *		stuff and a semaphore. sem is very useful to protect all
 *		the elements of this structure when multiple threads are
 *		trying to access these variables.
 */
struct mqtt_app_ctx_t {
	struct mqtt_client_ctx_t *client;
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
	struct nano_sem sem;

	int (*cb_publish)(struct mqtt_msg_t *msg);

};

/**
 * @brief MQTT_APP_INIT		MQTT application structure initializer
 *
 * @details			This define initializes the MQTT
 *				application structure.
 *				However, the semaphore is not initialized.
 *				See the mqtt_init function.
 */
#define MQTT_APP_INIT(cl, net, tx, rx) {	.client = cl,		\
						.netz_ctx = net,	\
						.tx_buf = tx,		\
						.cb_publish = NULL,	\
						.rx_buf = rx}

/**
 * @brief mqtt_cb_publish	Installs a callback, processed when a
 *				PUBLISH message is received
 * @param app			MQTT Application Context structure
 * @return			0, always
 */
int mqtt_cb_publish(struct mqtt_app_ctx_t *app,
		    int (*cb_publish)(struct mqtt_msg_t *msg));


/*
 *	****************************************************************
 *	MQTT configuration functions. No network activity involved here.
 */

/**
 * @brief mqtt_init		Initializes runtime components
 * @details			So far, this function only initializes
 *				the semaphore.
 * @param app			MQTT application context.
 * @return			0, always
 */
int mqtt_init(struct mqtt_app_ctx_t *app);

/**
 * @brief mqtt_configure	MQTT connection parameters configuration
 * @details			Function arguments are passed to the
 *				MQTT client structure: app->client.
 * @param app			MQTT application context.
 * @param client_id		C-string containing the client name.
 * @param clean_session		So far, only clean_session = 1 is
 *				supported.
 * @param proto			MQTT protocol: MQTT_311.
 * @return			0, always.
 */
int mqtt_configure(struct mqtt_app_ctx_t *app, const char *client_id,
		   int clean_session, enum mqtt_protocol proto);

/**
 * @brief mqtt_auth		Authentication parameters configuration
 * @details			This function sets the username and pass
 *				parameters used in applications that
 *				authenticate against the MQTT server.
  * @param app			MQTT application context.
 * @param username		C-string
 * @param pass			C-string
 * @return			0, always
 */
int mqtt_auth(struct mqtt_app_ctx_t *app, const char *username,
	      const char *pass);

/**
 * @brief mqtt_will		Application will message configuration
 * @details			This functions sets the will message
 *				parameters. <b>The use of this function
 *				is optional</b>.
 *				For more information read the MQTT
 *				specification.
 * @param app			MQTT application context.
 * @param topic			Topic of the will message.
 * @param msg			Message that will be broadcasted.
 * @param qos			Desired QoS of the will message.
 * @param retained		Retained property.
 * @return			0, always.
 */
int mqtt_will(struct mqtt_app_ctx_t *app, const char *topic,
	      const char *msg, enum mqtt_qos qos, int retained);

/**
 * @brief mqtt_buffers		Sets the application buffers.
 * @details			This function takes two buffers that
 *				will be used in networking operations.
 * @param app			MQTT application context.
 * @param tx_buf		Transmission buffer.
 * @param rx_buf		Reception buffer
 * @return			0, always.
 */
int mqtt_buffers(struct mqtt_app_ctx_t *app,
		 struct app_buf_t *tx_buf, struct app_buf_t *rx_buf);

/**
 * @brief mqtt_network		Sets the network context.
 * @param app			MQTT application context.
 * @param netz_ctx		Network context.
 * @return			0, always.
 */
int mqtt_network(struct mqtt_app_ctx_t *app, struct netz_ctx_t *netz_ctx);

/*
 *	             End of MQTT configuration functions.
 *	****************************************************************
 */



/*
 *	****************************************************************
 *		MQTT routines that require network functionality.
 */


/**
 * @brief mqtt_connect		Sends the MQTT CONNECT message.
 * @details			This function packs and sends the MQTT
 *				CONNECT message to the server.
 * @param app			MQTT application context.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_connect(struct mqtt_app_ctx_t *app);

/**
 * @brief mqtt_disconnect	Sends the MQTT DISCONNECT message.
 * @details			This function packs and sends the MQTT
 *				DISCONNECT message to the server.
 * @param app			MQTT application context.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_disconnect(struct mqtt_app_ctx_t *app);

/**
 * @brief mqtt_publish		Sends the MQTT PUBLISH message.
 * @details			This function packs and sends the MQTT
 *				PUBLISH message to the server.
 *				See the mqtt_msg_topic and
 *				mqtt_msg_payload_str functions.
 * @param app			MQTT application context.
 * @param msg			MQTT Message structure.
 * @param qos			Message QoS.
 * @param retained		Retained property.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_publish(struct mqtt_app_ctx_t *app, struct mqtt_msg_t *msg,
		 enum mqtt_qos qos, int retained);

/**
 * @brief mqtt_pingreq		Sends the MQTT PINGREQ message.
 * @details			This function packs and sends the MQTT
 *				PINGREQ message to the server.
 * @param app			MQTT application context.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_pingreq(struct mqtt_app_ctx_t *app);

/**
 * @brief mqtt_subscribe	Sends the MQTT SUBSCRIBE message.
 * @details			This functions packs and sends the MQTT
 *				SUBSCRIBE message to the server.
 *				So far, only one topic is processed by
 *				message.
 *				TODO: add an array of topics/qos.
 * @param app			MQTT application context.
 * @param topic			Topic to subscribe to.
 * @param qos			The desired QoS.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_subscribe(struct mqtt_app_ctx_t *app, char *topic, enum mqtt_qos qos);

/**
 * @brief mqtt_unsubscribe	Sends the MQTT UNSUBSCRIBE message
 * @details			This functions packs and sends the MQTT
 *				UNSUBSCRIBE message to the server.
 *				So far, only one topic is processed by
 *				message.
 *				TODO: add an array of topics.
 * @param app			MQTT application context.
 * @param topic			Topic to unsubscribe from.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_unsubscribe(struct mqtt_app_ctx_t *app, char *topic);

/**
 * @brief mqtt_read		Read any received MQTT message
 * @details			If a MQTT PUBLISH message is received,
 *				this function handles in background the
 *				response (if any).
 *				TODO: implement more messages' handlers.
 * @param app			MQTT application context.
 * @return			0 on success.
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 * @return			-EIO on network error.
 */
int mqtt_read(struct mqtt_app_ctx_t *app);

/*
 *	         End of MQTT functions with network support.
 *	****************************************************************
 */

#endif
