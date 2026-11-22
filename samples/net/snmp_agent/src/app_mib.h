/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_MIB_H_
#define APP_MIB_H_

#ifdef __cplusplus
extern "C" {
#endif

struct snmp_agent;

/**
 * @brief Initialize and register the Private MIB for the sample on one agent:
 *        1.3.6.1.4.1.<enterprise>.1 (nucleoH743)
 *        - device (1)
 *          - sensorValue (1.0)
 *          - ledState (2.0)
 *          - buttonCounter (3.0)
 *          - sensorHumidity (4.0)
 *          - sensorDescription (5.0)
 *          - sensorStatusOid (6.0)
 *        - sensorTable (3)
 *
 *        Safe to call for more than one agent: the node structs are copied into
 *        each agent's own tree and the shared sensorTable row list is linked only
 *        once (before any listener runs).
 * @param agent Agent instance whose MIB tree receives the nodes. Caller-owned.
 * @return 0 on success, negative errno on failure.
 */
int app_mib_init(struct snmp_agent *agent);

#ifdef __cplusplus
}
#endif

#endif /* APP_MIB_H_ */
