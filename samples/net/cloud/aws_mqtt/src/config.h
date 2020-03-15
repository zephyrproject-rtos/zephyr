/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define ALIVE_TIME      (MSEC_PER_SEC * 60U)
#define APP_MQTT_BUFFER_SIZE    128
#error "Fill the Host Name and compile"
#define CONFIG_AWS_HOSTNAME ""
#define CONFIG_AWS_PORT         8883
#define MQTT_CLIENTID           "zephyr_publisher"

#endif /* __CONFIG_H__ */

