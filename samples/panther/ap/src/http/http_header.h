/**
 * \file
 *
 * \brief HTTP header defines.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \addtogroup sam0_httpc_group
 * @{
 */

#ifndef HTTP_HEADER_H_INCLUDE
#define HTTP_HEADER_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_HEADER_ACCEPT       	    ("Accept: ")
#define HTTP_HEADER_ACCEPT_CHARSET   	("Accept-Charset: ")
#define HTTP_HEADER_ACCEPT_ENCODING  	("Accept-Encoding: ")
#define HTTP_HEADER_ACCEPT_LANGUAGE  	("Accept-Language: ")
#define HTTP_HEADER_ACCEPT_RANGES    	("Accept-Ranges: ")
#define HTTP_HEADER_AGE              	("Age: ")
#define HTTP_HEADER_ALLOW	            ("Allow: ")
#define HTTP_HEADER_AUTHORIZATION    	("Authorization: ")
#define HTTP_HEADER_CACHE_CONTROL    	("Cache-Control: ")
#define HTTP_HEADER_CONNECTION       	("Connection: ")
#define HTTP_HEADER_CONTENT_ENCODING 	("Content-Encoding: ")
#define HTTP_HEADER_CONTENT_LANGUAGE 	("Content-Language: ")
#define HTTP_HEADER_CONTENT_LENGTH   	("Content-Length: ")
#define HTTP_HEADER_CONTENT_LOCATION 	("Content-Location: ")
#define HTTP_HEADER_CONTENT_MD5      	("Content-MD5: ")
#define HTTP_HEADER_CONTENT_RANGE        ("Content-Range: ")
#define HTTP_HEADER_CONTENT_TYPE         ("Content-Type: ")
#define HTTP_HEADER_DATE                 ("Date: ")
#define HTTP_HEADER_ETAG                 ("ETag: ")
#define HTTP_HEADER_EXPECT               ("Expect: ")
#define HTTP_HEADER_EXPIRES              ("Expires: ")
#define HTTP_HEADER_FROM                 ("From: ")
#define HTTP_HEADER_HOST                 ("Host: ")
#define HTTP_HEADER_IF_MATCH             ("If-Match: ")
#define HTTP_HEADER_IF_MODIFIED_SINCE	("If-Modified-Since: ")
#define HTTP_HEADER_IF_NONE_MATCH        ("If-None-Match: ")
#define HTTP_HEADER_IF_RANGE             ("If-Range: ")
#define HTTP_HEADER_IF_UNMODIFIED_SINCE	("If-Unmodified-Since: ")
#define HTTP_HEADER_LAST_MODIFIED        ("Last-Modified: ")
#define HTTP_HEADER_LOCATION             ("Location: ")
#define HTTP_HEADER_MAX_FORWARDS         ("Max-Forwards: ")
#define HTTP_HEADER_PRAGMA               ("Pragma: ")
#define HTTP_HEADER_PROXY_AUTHENTICATE	("Proxy-Authenticate: ")
#define HTTP_HEADER_PROXY_AUTHORIZATION  ("Proxy-Authorization: ")
#define HTTP_HEADER_RANGE                ("Range: ")
#define HTTP_HEADER_REFERER              ("Referer: ")
#define HTTP_HEADER_RETRY_AFTER          ("Retry-After: ")
#define HTTP_HEADER_SERVER               ("Server: ")
#define HTTP_HEADER_TE                   ("TE: ")
#define HTTP_HEADER_TRAILER              ("Trailer: ")
#define HTTP_HEADER_TRANSFER_ENCODING	("Transfer-Encoding: ")
#define HTTP_HEADER_UPGRADE              ("Upgrade: ")
#define HTTP_HEADER_USER_AGENT           ("User-Agent: ")
#define HTTP_HEADER_VARY                 ("Vary: ")
#define HTTP_HEADER_VIA                  ("Via: ")
#define HTTP_HEADER_WARNING              ("Warning: ")
#define HTTP_HEADER_WWW_AUTHENTICATE     ("WWW-Authenticate: ")

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HTTP_HEADER_H_INCLUDE */