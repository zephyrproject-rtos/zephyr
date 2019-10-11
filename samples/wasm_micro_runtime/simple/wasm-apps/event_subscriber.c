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

void over_heat_event_handler(request_t *request)
{
    printf("### user over heat event handler called\n");

    if (request->payload != NULL && request->fmt == FMT_ATTR_CONTAINER)
        attr_container_dump((attr_container_t *) request->payload);
}

void on_init()
{
    api_subscribe_event("alert/overheat", over_heat_event_handler);
}

void on_destroy()
{
    /* real destroy work including killing timer and closing sensor is
       accomplished in wasm app library version of on_destroy() */
}
