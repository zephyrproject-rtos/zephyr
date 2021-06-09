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

#ifndef H_MCUMGR_UTIL_
#define H_MCUMGR_UTIL_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts an unsigned long long to a null-terminated string.
 *
 * @param val                   The source number to convert.
 * @param dst_max_len           The size, in bytes, of the destination buffer.
 * @param dst                   The destination buffer.
 *
 * @return                      The length of the resulting string on success;
 *                              -1 if the buffer is too small.
 */
int ull_to_s(unsigned long long val, int dst_max_len, char *dst);

/**
 * @brief Converts a long long to a null-terminated string.
 *
 * @param val                   The source number to convert.
 * @param dst_max_len           The size, in bytes, of the destination buffer.
 * @param dst                   The destination buffer.
 *
 * @return                      The length of the resulting string on success;
 *                              -1 if the buffer is too small.
 */
int ll_to_s(long long val, int dst_max_len, char *dst);

#ifdef __cplusplus
}
#endif

#endif
