/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

service Hello {
    void ping();
    string echo(1: string msg);
    i32 counter();
}
