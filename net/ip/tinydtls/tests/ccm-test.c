#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef WITH_CONTIKI
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#endif /* WITH_CONTIKI */

//#include "debug.h"
#include "numeric.h"
#include "ccm.h"

#include "ccm-testdata.c"

#ifndef HAVE_FLS
int fls(unsigned int i) {
  int n;
  for (n = 0; i; n++)
    i >>= 1;
  return n;
}
#endif

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

#ifdef WITH_CONTIKI
PROCESS(ccm_test_process, "CCM test process");
AUTOSTART_PROCESSES(&ccm_test_process);
PROCESS_THREAD(ccm_test_process, ev, d)
{
#else  /* WITH_CONTIKI */
int main(int argc, char **argv) {
#endif /* WITH_CONTIKI */
  long int len;
  int n;

  rijndael_ctx ctx;

#ifdef WITH_CONTIKI
  PROCESS_BEGIN();
#endif /* WITH_CONTIKI */

  for (n = 0; n < sizeof(data)/sizeof(struct test_vector); ++n) {

    if (rijndael_set_key_enc_only(&ctx, data[n].key, 8*sizeof(data[n].key)) < 0) {
      fprintf(stderr, "cannot set key\n");
      return -1;
    }

    len = dtls_ccm_encrypt_message(&ctx, data[n].M, data[n].L, data[n].nonce, 
				   data[n].msg + data[n].la, 
				   data[n].lm - data[n].la, 
				   data[n].msg, data[n].la);
    
    len +=  + data[n].la;
    printf("Packet Vector #%d ", n+1);
    if (len != data[n].r_lm || memcmp(data[n].msg, data[n].result, len))
      printf("FAILED, ");
    else 
      printf("OK, ");
    
    printf("result is (total length = %lu):\n\t", len);
    dump(data[n].msg, len);

    len = dtls_ccm_decrypt_message(&ctx, data[n].M, data[n].L, data[n].nonce, 
				   data[n].msg + data[n].la, len - data[n].la, 
				   data[n].msg, data[n].la);
    
    if (len < 0)
      printf("Packet Vector #%d: cannot decrypt message\n", n+1);
    else 
      printf("\t*** MAC verified (total length = %lu) ***\n", len + data[n].la);
  }

#ifdef WITH_CONTIKI
  PROCESS_END();
#else /* WITH_CONTIKI */
  return 0;
#endif /* WITH_CONTIKI */
}
