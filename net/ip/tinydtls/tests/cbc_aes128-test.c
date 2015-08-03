#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "numeric.h"
#include "crypto.h"

#include "cbc_aes128-testdata.c"

void 
dump(unsigned char *buf, size_t len) {
  size_t i = 0;
  while (i < len) {
    printf("%02x ", buf[i++]);
    if (i % 4 == 0)
      printf(" ");
    if (i % 16 == 0)
      printf("\n\t");
  }
  printf("\n");
}

int main(int argc, char **argv) {
  int len, n;

  for (n = 0; n < sizeof(data)/sizeof(struct test_vector); ++n) {
    dtls_cipher_context_t *cipher;

    cipher = dtls_new_cipher(&ciphers[AES128],
			     data[n].key,
			     sizeof(data[n].key));
    
    if (!cipher) {
      fprintf(stderr, "cannot set key\n");
      exit(-1);
    }

    dtls_init_cipher(cipher, data[n].nonce, sizeof(data[n].nonce));

    if (data[n].M == 0)
      len = dtls_encrypt(cipher, data[n].msg, data[n].lm);
    else
      len = dtls_decrypt(cipher, data[n].msg, data[n].lm);

    printf("Packet Vector #%d ", n+1);
    if (len != data[n].r_lm
	|| memcmp(data[n].msg, data[n].result, len))
      printf("FAILED, ");
    else 
      printf("OK, ");
    
    printf("result is (total length = %d):\n\t", (int)len);
    dump(data[n].msg, len);

    free(cipher);
  }

  return 0;
}
