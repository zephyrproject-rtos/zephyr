/*
 * Copyright (c) 2019 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#if defined(CONFIG_UPDATEHUB_DTLS)
#define CA_CERTIFICATE_TAG 1

static const unsigned char server_certificate[] = {
    #include "servercert.der.inc"
};


static const unsigned char private_key[] = {
    #include "privkey.der.inc"
};

#endif
#endif /* __CERTIFICATE_H__ */
