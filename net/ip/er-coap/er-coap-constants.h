/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Collection of constants specified in the CoAP standard.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef ER_COAP_CONSTANTS_H_
#define ER_COAP_CONSTANTS_H_

#define COAP_DEFAULT_PORT                    5683
#define COAP_DEFAULT_SECURE_PORT             5684

#define COAP_DEFAULT_MAX_AGE                 60
#define COAP_RESPONSE_TIMEOUT                3
#define COAP_RESPONSE_RANDOM_FACTOR          1.5
#define COAP_RESPONSE_RANDOM_FACTOR_INT      15
#define COAP_MAX_RETRANSMIT                  4

#define COAP_HEADER_LEN                      4  /* | version:0x03 type:0x0C tkl:0xF0 | code | mid:0x00FF | mid:0xFF00 | */
#define COAP_TOKEN_LEN                       8  /* The maximum number of bytes for the Token */
#define COAP_ETAG_LEN                        8  /* The maximum number of bytes for the ETag */

#define COAP_HEADER_VERSION_MASK             0xC0
#define COAP_HEADER_VERSION_POSITION         6
#define COAP_HEADER_TYPE_MASK                0x30
#define COAP_HEADER_TYPE_POSITION            4
#define COAP_HEADER_TOKEN_LEN_MASK           0x0F
#define COAP_HEADER_TOKEN_LEN_POSITION       0

#define COAP_HEADER_OPTION_DELTA_MASK        0xF0
#define COAP_HEADER_OPTION_SHORT_LENGTH_MASK 0x0F

/* CoAP message types */
typedef enum {
  COAP_TYPE_CON,                /* confirmables */
  COAP_TYPE_NON,                /* non-confirmables */
  COAP_TYPE_ACK,                /* acknowledgements */
  COAP_TYPE_RST                 /* reset */
} coap_message_type_t;

/* CoAP request method codes */
typedef enum {
  COAP_GET = 1,
  COAP_POST,
  COAP_PUT,
  COAP_DELETE
} coap_method_t;

/* CoAP response codes */
typedef enum {
  NO_ERROR = 0,

  CREATED_2_01 = 65,            /* CREATED */
  DELETED_2_02 = 66,            /* DELETED */
  VALID_2_03 = 67,              /* NOT_MODIFIED */
  CHANGED_2_04 = 68,            /* CHANGED */
  CONTENT_2_05 = 69,            /* OK */
  CONTINUE_2_31 = 95,           /* CONTINUE */

  BAD_REQUEST_4_00 = 128,       /* BAD_REQUEST */
  UNAUTHORIZED_4_01 = 129,      /* UNAUTHORIZED */
  BAD_OPTION_4_02 = 130,        /* BAD_OPTION */
  FORBIDDEN_4_03 = 131,         /* FORBIDDEN */
  NOT_FOUND_4_04 = 132,         /* NOT_FOUND */
  METHOD_NOT_ALLOWED_4_05 = 133,        /* METHOD_NOT_ALLOWED */
  NOT_ACCEPTABLE_4_06 = 134,    /* NOT_ACCEPTABLE */
  PRECONDITION_FAILED_4_12 = 140,       /* BAD_REQUEST */
  REQUEST_ENTITY_TOO_LARGE_4_13 = 141,  /* REQUEST_ENTITY_TOO_LARGE */
  UNSUPPORTED_MEDIA_TYPE_4_15 = 143,    /* UNSUPPORTED_MEDIA_TYPE */

  INTERNAL_SERVER_ERROR_5_00 = 160,     /* INTERNAL_SERVER_ERROR */
  NOT_IMPLEMENTED_5_01 = 161,   /* NOT_IMPLEMENTED */
  BAD_GATEWAY_5_02 = 162,       /* BAD_GATEWAY */
  SERVICE_UNAVAILABLE_5_03 = 163,       /* SERVICE_UNAVAILABLE */
  GATEWAY_TIMEOUT_5_04 = 164,   /* GATEWAY_TIMEOUT */
  PROXYING_NOT_SUPPORTED_5_05 = 165,    /* PROXYING_NOT_SUPPORTED */

  /* Erbium errors */
  MEMORY_ALLOCATION_ERROR = 192,
  PACKET_SERIALIZATION_ERROR,

  /* Erbium hooks */
  MANUAL_RESPONSE,
  PING_RESPONSE
} coap_status_t;

/* CoAP header option numbers */
typedef enum {
  COAP_OPTION_IF_MATCH = 1,     /* 0-8 B */
  COAP_OPTION_URI_HOST = 3,     /* 1-255 B */
  COAP_OPTION_ETAG = 4,         /* 1-8 B */
  COAP_OPTION_IF_NONE_MATCH = 5,        /* 0 B */
  COAP_OPTION_OBSERVE = 6,      /* 0-3 B */
  COAP_OPTION_URI_PORT = 7,     /* 0-2 B */
  COAP_OPTION_LOCATION_PATH = 8,        /* 0-255 B */
  COAP_OPTION_URI_PATH = 11,    /* 0-255 B */
  COAP_OPTION_CONTENT_FORMAT = 12,      /* 0-2 B */
  COAP_OPTION_MAX_AGE = 14,     /* 0-4 B */
  COAP_OPTION_URI_QUERY = 15,   /* 0-255 B */
  COAP_OPTION_ACCEPT = 17,      /* 0-2 B */
  COAP_OPTION_LOCATION_QUERY = 20,      /* 0-255 B */
  COAP_OPTION_BLOCK2 = 23,      /* 1-3 B */
  COAP_OPTION_BLOCK1 = 27,      /* 1-3 B */
  COAP_OPTION_SIZE2 = 28,       /* 0-4 B */
  COAP_OPTION_PROXY_URI = 35,   /* 1-1034 B */
  COAP_OPTION_PROXY_SCHEME = 39,        /* 1-255 B */
  COAP_OPTION_SIZE1 = 60,       /* 0-4 B */
} coap_option_t;

/* CoAP Content-Formats */
typedef enum {
  TEXT_PLAIN = 0,
  TEXT_XML = 1,
  TEXT_CSV = 2,
  TEXT_HTML = 3,
  IMAGE_GIF = 21,
  IMAGE_JPEG = 22,
  IMAGE_PNG = 23,
  IMAGE_TIFF = 24,
  AUDIO_RAW = 25,
  VIDEO_RAW = 26,
  APPLICATION_LINK_FORMAT = 40,
  APPLICATION_XML = 41,
  APPLICATION_OCTET_STREAM = 42,
  APPLICATION_RDF_XML = 43,
  APPLICATION_SOAP_XML = 44,
  APPLICATION_ATOM_XML = 45,
  APPLICATION_XMPP_XML = 46,
  APPLICATION_EXI = 47,
  APPLICATION_FASTINFOSET = 48,
  APPLICATION_SOAP_FASTINFOSET = 49,
  APPLICATION_JSON = 50,
  APPLICATION_X_OBIX_BINARY = 51
} coap_content_format_t;

#endif /* ER_COAP_CONSTANTS_H_ */
