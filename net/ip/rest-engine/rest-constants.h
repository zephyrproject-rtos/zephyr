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
 *      Constants for the REST Engine (Erbium).
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef REST_CONSTANTS_H_
#define REST_CONSTANTS_H_

/**
 * Generic status codes that are mapped to either HTTP or CoAP codes.
 */
struct rest_implementation_status {
  const unsigned int OK;                        /* CONTENT_2_05,                  OK_200 */
  const unsigned int CREATED;                   /* CREATED_2_01,                  CREATED_201 */
  const unsigned int CHANGED;                   /* CHANGED_2_04,                  NO_CONTENT_204 */
  const unsigned int DELETED;                   /* DELETED_2_02,                  NO_CONTENT_204 */
  const unsigned int NOT_MODIFIED;              /* VALID_2_03,                    NOT_MODIFIED_304 */

  const unsigned int BAD_REQUEST;               /* BAD_REQUEST_4_00,              BAD_REQUEST_400 */
  const unsigned int UNAUTHORIZED;              /* UNAUTHORIZED_4_01,             UNAUTHORIZED_401 */
  const unsigned int BAD_OPTION;                /* BAD_OPTION_4_02,               BAD_REQUEST_400 */
  const unsigned int FORBIDDEN;                 /* FORBIDDEN_4_03,                FORBIDDEN_403 */
  const unsigned int NOT_FOUND;                 /* NOT_FOUND_4_04,                NOT_FOUND_404 */
  const unsigned int METHOD_NOT_ALLOWED;        /* METHOD_NOT_ALLOWED_4_05,       METHOD_NOT_ALLOWED_405 */
  const unsigned int NOT_ACCEPTABLE;            /* NOT_ACCEPTABLE_4_06,           NOT_ACCEPTABLE_406 */
  const unsigned int REQUEST_ENTITY_TOO_LARGE;  /* REQUEST_ENTITY_TOO_LARGE_4_13, REQUEST_ENTITY_TOO_LARGE_413 */
  const unsigned int UNSUPPORTED_MEDIA_TYPE;    /* UNSUPPORTED_MEDIA_TYPE_4_15,   UNSUPPORTED_MEDIA_TYPE_415 */

  const unsigned int INTERNAL_SERVER_ERROR;     /* INTERNAL_SERVER_ERROR_5_00,    INTERNAL_SERVER_ERROR_500 */
  const unsigned int NOT_IMPLEMENTED;           /* NOT_IMPLEMENTED_5_01,          NOT_IMPLEMENTED_501 */
  const unsigned int BAD_GATEWAY;               /* BAD_GATEWAY_5_02,              BAD_GATEWAY_502 */
  const unsigned int SERVICE_UNAVAILABLE;       /* SERVICE_UNAVAILABLE_5_03,      SERVICE_UNAVAILABLE_503 */
  const unsigned int GATEWAY_TIMEOUT;           /* GATEWAY_TIMEOUT_5_04,          GATEWAY_TIMEOUT_504 */
  const unsigned int PROXYING_NOT_SUPPORTED;    /* PROXYING_NOT_SUPPORTED_5_05,   INTERNAL_SERVER_ERROR_500 */
};

/**
 * List of Content-Formats which are Internet Media Types plus encoding.
 * TODO This should be a constant enum taken from CoAP for both CoAP and HTTP.
 */
struct rest_implementation_type {
  unsigned int TEXT_PLAIN;
  unsigned int TEXT_XML;
  unsigned int TEXT_CSV;
  unsigned int TEXT_HTML;
  unsigned int IMAGE_GIF;
  unsigned int IMAGE_JPEG;
  unsigned int IMAGE_PNG;
  unsigned int IMAGE_TIFF;
  unsigned int AUDIO_RAW;
  unsigned int VIDEO_RAW;
  unsigned int APPLICATION_LINK_FORMAT;
  unsigned int APPLICATION_XML;
  unsigned int APPLICATION_OCTET_STREAM;
  unsigned int APPLICATION_RDF_XML;
  unsigned int APPLICATION_SOAP_XML;
  unsigned int APPLICATION_ATOM_XML;
  unsigned int APPLICATION_XMPP_XML;
  unsigned int APPLICATION_EXI;
  unsigned int APPLICATION_FASTINFOSET;
  unsigned int APPLICATION_SOAP_FASTINFOSET;
  unsigned int APPLICATION_JSON;
  unsigned int APPLICATION_X_OBIX_BINARY;
};

/**
 * Resource flags for allowed methods and special functionalities.
 */
typedef enum {
  NO_FLAGS = 0,

  /* methods to handle */
  METHOD_GET = (1 << 0),
  METHOD_POST = (1 << 1),
  METHOD_PUT = (1 << 2),
  METHOD_DELETE = (1 << 3),

  /* special flags */
  HAS_SUB_RESOURCES = (1 << 4),
  IS_SEPARATE = (1 << 5),
  IS_OBSERVABLE = (1 << 6),
  IS_PERIODIC = (1 << 7)
} rest_resource_flags_t;

#endif /* REST_CONSTANTS_H_ */
