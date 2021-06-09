/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_STAT_MGMT_
#define H_STAT_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for statistics management group.
 */
#define STAT_MGMT_ID_SHOW   0
#define STAT_MGMT_ID_LIST   1

/**
 * @brief Represents a single value in a statistics group.
 */
struct stat_mgmt_entry {
    const char *name;
    uint64_t value;
};

/**
 * @brief Registers the statistics management command handler group.
 */ 
void stat_mgmt_register_group(void);

#ifdef __cplusplus
}
#endif

#endif /* H_STAT_MGMT_ */
