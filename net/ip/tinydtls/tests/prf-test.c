#include <stdio.h>

#include "tinydtls.h"
#include "debug.h"
#include "global.h"
#include "crypto.h"

int 
main() {
  /* see http://www.ietf.org/mail-archive/web/tls/current/msg03416.html */
  unsigned char key[] = { 0x9b, 0xbe, 0x43, 0x6b, 0xa9, 0x40, 0xf0, 0x17, 
			  0xb1, 0x76, 0x52, 0x84, 0x9a, 0x71, 0xdb, 0x35 };
  unsigned char label[] = { 0x74, 0x65, 0x73, 0x74, 0x20, 0x6c, 0x61, 0x62, 
			    0x65, 0x6c};
  unsigned char random1[] = { 0xa0, 0xba, 0x9f, 0x93, 0x6c, 0xda, 0x31, 0x18};
  unsigned char random2[] = {0x27, 0xa6, 0xf7, 0x96, 0xff, 0xd5, 0x19, 0x8c
  };
  unsigned char buf[200];
  size_t result;
  
  result = dtls_prf(key, sizeof(key),
		    label, sizeof(label),
		    random1, sizeof(random1),
		    random2, sizeof(random2),
		    buf, 100);

  printf("PRF yields %zu bytes of random data:\n", result);
  hexdump(buf, result);
  printf("\n");
  return 0;
}
