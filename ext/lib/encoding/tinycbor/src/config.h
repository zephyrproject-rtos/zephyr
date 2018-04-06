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

#ifndef TINYCBOR_CONFIG_H
#define TINYCBOR_CONFIG_H

/** This option specifies whether a default writer exists **/
#ifdef CONFIG_CBOR_NO_DFLT_WRITER
#define CBOR_NO_DFLT_WRITER CONFIG_CBOR_NO_DFLT_WRITER
#endif

/** This option specifies whether a default reader exists **/
#ifdef CONFIG_CBOR_NO_DFLT_READER
#define CBOR_NO_DFLT_READER CONFIG_CBOR_NO_DFLT_READER
#endif

/** This option specifies whether a check user exists for a cbor encoder **/
#ifdef CONFIG_CBOR_ENCODER_NO_CHECK_USER
#define CBOR_ENCODER_NO_CHECK_USER CONFIG_CBOR_ENCODER_NO_CHECK_USER
#endif

/** This option specifies max recursions for the parser **/
#ifdef CONFIG_CBOR_PARSER_MAX_RECURSIONS
#define CBOR_PARSER_MAX_RECURSIONS CONFIG_CBOR_PARSER_MAX_RECURSIONS
#endif

/** This option enables the strict parser checks **/
#ifdef CONFIG_CBOR_PARSER_NO_STRICT_CHECKS
#define CBOR_PARSER_NO_STRICT_CHECKS CONFIG_CBOR_PARSER_NO_STRICT_CHECKS
#endif

/** This option enables floating point support **/
#ifndef CONFIG_CBOR_FLOATING_POINT
#define CBOR_NO_FLOATING_POINT 1
#endif

/** This option enables half float type support **/
#ifndef CONFIG_CBOR_HALF_FLOAT_TYPE
#define CBOR_NO_HALF_FLOAT_TYPE 1
#endif

/** This option enables open memstream support **/
#ifdef CONFIG_CBOR_WITHOUT_OPEN_MEMSTREAM
#define CBOR_WITHOUT_OPEN_MEMSTREAM CONFIG_CBOR_WITHOUT_OPEN_MEMSTREAM
#endif

#endif /* TINYCBOR_CONFIG_H */
