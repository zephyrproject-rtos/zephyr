Copyright 2022 TU Wien

	Licensed under the Apache License,
	Version 2.0(the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

#ifndef __MPL_H
#define __MPL_H

#include <zephyr/types.h>

#include <net/net_pkt.h>

#define ALL_MPL_FORWARDERS    "FF03::FC"
#define ALL_MPL_FORWARDERS_LL "FF02::FC"

#define MPL_OPTION_BASE_LENGTH 8
#define HBHO_BASE_LEN	       8

#define HBHO_S0_LEN    0
#define HBHO_S1_LEN    0
#define HBHO_S2_LEN    8
#define HBHO_S3_LEN    16
#define MPL_OPT_LEN_S0 2
#define MPL_OPT_LEN_S1 4
#define MPL_OPT_LEN_S2 10
#define MPL_OPT_LEN_S3 18

#define HBHO_OPT_TYPE_MPL 0x6D

int net_route_mpl_accept(struct net_pkt *pkt, uint8_t is_input);
void net_route_mpl_send_data(struct net_pkt *pkt);

#endif /* __MPL_H */
