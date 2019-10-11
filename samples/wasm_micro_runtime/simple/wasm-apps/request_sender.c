/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm_app.h"

static void my_response_handler(response_t *response, void *user_data)
{
    char *tag = (char *) user_data;

    if (response == NULL) {
        printf("[req] request timeout!\n");
        return;
    }

    printf("[req] response handler called mid:%d, status:%d, fmt:%d, payload:%p, len:%d, tag:%s\n",
           response->mid, response->status, response->fmt, response->payload,
           response->payload_len, tag);

    if (response->payload != NULL
        && response->payload_len > 0
        && response->fmt == FMT_ATTR_CONTAINER) {
        printf("[req] dump the response payload:\n");
        attr_container_dump((attr_container_t *) response->payload);
    }
}

static void test_send_request(char *url, char *tag)
{
    request_t request[1];

    init_request(request, url, COAP_PUT, 0, NULL, 0);
    api_send_request(request, my_response_handler, tag);
}

void on_init()
{
    test_send_request("/app/request_handler/url1", "a request to target app");
    test_send_request("url1", "a general request");
}

void on_destroy()
{
    /* real destroy work including killing timer and closing sensor is
       accomplished in wasm app library version of on_destroy() */
}
