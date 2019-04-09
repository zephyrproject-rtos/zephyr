/******************************************************************************
*  Filename:       sw_poly1305-donna.h
*  Revised:        2016-10-05 12:42:03 +0200 (Wed, 05 Oct 2016)
*  Revision:       47308
******************************************************************************/

#ifndef POLY1305_DONNA_H
#define POLY1305_DONNA_H

#include <stddef.h>

typedef struct {
   size_t aligner;
   unsigned char opaque[136];
} poly1305_context;

void poly1305_init(poly1305_context *ctx, const unsigned char key[32]);
void poly1305_update(poly1305_context *ctx, const unsigned char *m, size_t bytes);
void poly1305_finish(poly1305_context *ctx, unsigned char mac[16]);
void poly1305_auth(unsigned char mac[16], const unsigned char *m, size_t bytes, const unsigned char key[32]);

int poly1305_verify(const unsigned char mac1[16], const unsigned char mac2[16]);
int poly1305_power_on_self_test(void);

#endif /* POLY1305_DONNA_H */
