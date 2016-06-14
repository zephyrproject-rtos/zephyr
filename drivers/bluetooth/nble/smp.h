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

void bt_smp_connected(struct bt_conn *conn);
void bt_smp_disconnected(struct bt_conn *conn);
int bt_smp_init(void);

int bt_smp_auth_cancel(struct bt_conn *conn);
int bt_smp_auth_pairing_confirm(struct bt_conn *conn);
int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);

int bt_smp_send_security_req(struct bt_conn *conn);
int bt_smp_send_pairing_req(struct bt_conn *conn);
