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

#ifndef UDP_CONFIG_H_
#define UDP_CONFIG_H_

#define NETMASK0		255
#define NETMASK1		255
#define NETMASK2		255
#define NETMASK3		0

#define CLIENT_IPADDR0		192
#define CLIENT_IPADDR1		0
#define CLIENT_IPADDR2		2
#define CLIENT_IPADDR3		1

#define SERVER_IPADDR0		192
#define SERVER_IPADDR1		0
#define SERVER_IPADDR2		2
#define SERVER_IPADDR3		2

#define SERVER_PORT	4433
#define CLIENT_PORT	8484

#define UDP_RX_TIMEOUT 20
#define UDP_TX_TIMEOUT 10

#define CLIENT_IP_ADDR          { { { CLIENT_IPADDR0, CLIENT_IPADDR1,   \
				      CLIENT_IPADDR2, CLIENT_IPADDR3 } } }

#define SERVER_IP_ADDR          { { { SERVER_IPADDR0, SERVER_IPADDR1,   \
				      SERVER_IPADDR2, SERVER_IPADDR3 } } }

#endif
