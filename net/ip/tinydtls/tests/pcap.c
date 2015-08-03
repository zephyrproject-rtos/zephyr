#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <pcap/pcap.h>

#include "tinydtls.h"
#include "debug.h"
#include "dtls.h"

#define TRANSPORT_HEADER_SIZE (14+20+8) /* Ethernet + IP + UDP */

/* the pre_master_secret is generated from the PSK at startup */
unsigned char pre_master_secret[60];
size_t pre_master_len = 0;

unsigned char master_secret[DTLS_MASTER_SECRET_LENGTH];
size_t master_secret_len = 0;

dtls_security_parameters_t security_params[2]; 
int config = 0;
unsigned int epoch[2] = { 0, 0 };

#if DTLS_VERSION == 0xfeff
dtls_hash_t hs_hash[2];
#elif DTLS_VERSION == 0xfefd
dtls_hash_t hs_hash[1];
#endif

static inline void
update_hash(uint8 *record, size_t rlength, 
	    uint8 *data, size_t data_length) {
  int i;

  if (!hs_hash[0])
    return;

  for (i = 0; i < sizeof(hs_hash) / sizeof(dtls_hash_t *); ++i) {
    dtls_hash_update(hs_hash[i], data, data_length);
  }
}

static inline void
finalize_hash(uint8 *buf) {
#if DTLS_VERSION == 0xfeff
  unsigned char statebuf[sizeof(md5_state_t) + sizeof(SHA_CTX)];
#elif DTLS_VERSION == 0xfefd
  unsigned char statebuf[sizeof(SHA256_CTX)];
#endif

  if (!hs_hash[0])
    return;

  /* temporarily store hash status for roll-back after finalize */
#if DTLS_VERSION == 0xfeff
  memcpy(statebuf, hs_hash[0], sizeof(md5_state_t));
  memcpy(statebuf + sizeof(md5_state_t), 
	 hs_hash[1], 
	 sizeof(SHA_CTX));
#elif DTLS_VERSION == 0xfefd
  memcpy(statebuf, hs_hash[0], sizeof(statebuf));
#endif

  dtls_hash_finalize(buf, hs_hash[0]);
#if DTLS_VERSION == 0xfeff
  dtls_hash_finalize(buf + 16, hs_hash[1]);
#endif

  /* restore hash status */
#if DTLS_VERSION == 0xfeff
  memcpy(hs_hash[0], statebuf, sizeof(md5_state_t));
  memcpy(hs_hash[1], 
	 statebuf + sizeof(md5_state_t), 
	 sizeof(SHA_CTX));
#elif DTLS_VERSION == 0xfefd
  memcpy(hs_hash[0], statebuf, sizeof(statebuf));
#endif
}

static inline void
clear_hash() {
  int i;

  for (i = 0; i < sizeof(hs_hash) / sizeof(dtls_hash_t *); ++i)
    free(hs_hash[i]);
  memset(hs_hash, 0, sizeof(hs_hash));
}

#undef CURRENT_CONFIG
#undef OTHER_CONFIG
#undef SWITCH_CONFIG
#define CURRENT_CONFIG (&security_params[config])
#define OTHER_CONFIG   (&security_params[!(config & 0x01)])
#define SWITCH_CONFIG  (config = !(config & 0x01))

int
pcap_verify(dtls_security_parameters_t *sec,
	    int is_client, 
	    const unsigned char *record, size_t record_length,
	    const unsigned char *cleartext, size_t cleartext_length) {

  unsigned char mac[DTLS_HMAC_MAX];
  dtls_hmac_context_t hmac_ctx;
  int ok;

  if (cleartext_length < dtls_kb_digest_size(sec))
    return 0;

  dtls_hmac_init(&hmac_ctx, 
		 is_client 
		 ? dtls_kb_client_mac_secret(sec)
		 : dtls_kb_server_mac_secret(sec),
		 dtls_kb_mac_secret_size(sec));

  cleartext_length -= dtls_kb_digest_size(sec);

  /* calculate MAC even if padding is wrong */
  dtls_mac(&hmac_ctx, 
	   record, 		/* the pre-filled record header */
	   cleartext, cleartext_length,
	   mac);

  ok = memcmp(mac, cleartext + cleartext_length, 
	      dtls_kb_digest_size(sec)) == 0;
#ifndef NDEBUG
  printf("MAC (%s): ", ok ? "valid" : "invalid");
  dump(mac, dtls_kb_digest_size(sec));
  printf("\n");
#endif
  return ok;
}
		    
int
decrypt_verify(int is_client, const uint8 *packet, size_t length,
	       uint8 **cleartext, size_t *clen) {
  int res, ok = 0;
  dtls_cipher_context_t *cipher;

  static unsigned char buf[1000];
  
  switch (CURRENT_CONFIG->cipher) {
  case AES128:			/* TLS_PSK_WITH_AES128_CBC_SHA */
    *cleartext = buf;
    *clen = length - sizeof(dtls_record_header_t);

    if (is_client)
      cipher = CURRENT_CONFIG->read_cipher;
    else 
      cipher = CURRENT_CONFIG->write_cipher; 

    res = dtls_decrypt(cipher,
		       (uint8 *)packet + sizeof(dtls_record_header_t), *clen, 
		       buf, NULL, 0);

    if (res < 0) {
      warn("decryption failed!\n");
    } else {
      ok = pcap_verify(CURRENT_CONFIG, is_client, (uint8 *)packet, length, 
		       *cleartext, res);  

      if (ok)
	*clen = res - dtls_kb_digest_size(CURRENT_CONFIG);
    }
    break;
  default:			/* no cipher suite selected */
    *cleartext = (uint8 *)packet + sizeof(dtls_record_header_t);
    *clen = length - sizeof(dtls_record_header_t);
    
    ok = 1;
  }
  
  if (ok)
    printf("verify OK\n");
  else
    printf("verification failed!\n");
  return ok;
}

#define SKIP_ETH_HEADER(M,L) 			\
  if ((L) < 14)					\
    return;					\
  else {					\
    (M) += 14;					\
    (L) -= 14;					\
  }

#define SKIP_IP_HEADER(M,L)				\
  if (((M)[0] & 0xF0) == 0x40) {	/* IPv4 */	\
    (M) += (M[0] & 0x0F) * 4;				\
    (L) -= (M[0] & 0x0F) * 4;				\
  } else						\
    if (((M)[0] & 0xF0) == 0x60) { /* IPv6 */		\
      (M) += 40;					\
      (L) -= 40;					\
    } 

#define SKIP_UDP_HEADER(M,L) {			\
    (M) += 8;					\
    (L) -= 8;					\
  }

void
handle_packet(const u_char *packet, int length) {
  static int n = 0;
  static unsigned char initial_hello[] = { 
    0x16, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
  };
  uint8 *data; 
  size_t data_length, rlen;
  int i, res;
#if DTLS_VERSION == 0xfeff
#ifndef SHA1_DIGEST_LENGTH
#define SHA1_DIGEST_LENGTH 20
#endif
  uint8 hash_buf[16 + SHA1_DIGEST_LENGTH];
#elif DTLS_VERSION == 0xfefd
  uint8 hash_buf[SHA256_DIGEST_LENGTH];
#endif
#define verify_data_length 12
  int is_client;
  n++;

  SKIP_ETH_HEADER(packet, length);
  SKIP_IP_HEADER(packet, length);

  /* determine from port if this is a client */
  is_client = dtls_uint16_to_int(packet) != 20220;

  SKIP_UDP_HEADER(packet, length);

  while (length) {
    rlen = dtls_uint16_to_int(packet + 11) + sizeof(dtls_record_header_t);

    if (!rlen) {
      fprintf(stderr, "invalid length!\n");
      return;
    }

    /* skip packet if it is from a different epoch */
    if (dtls_uint16_to_int(packet + 3) != epoch[is_client])
      goto next;

    res = decrypt_verify(is_client, packet, rlen,
			 &data, &data_length);

    if (res <= 0)
      goto next;
    
    printf("packet %d (from %s):\n", n, is_client ? "client" : "server");
    hexdump(packet, sizeof(dtls_record_header_t));
    printf("\n");
    hexdump(data, data_length);
    printf("\n");
    
    if (packet[0] == 22 && data[0] == 1) { /* ClientHello */
      if (memcmp(packet, initial_hello, sizeof(initial_hello)) == 0)
	goto next;
	
      memcpy(dtls_kb_client_iv(OTHER_CONFIG), data + 14, 32);

	clear_hash();
#if DTLS_VERSION == 0xfeff
      hs_hash[0] = dtls_new_hash(HASH_MD5);
      hs_hash[1] = dtls_new_hash(HASH_SHA1);

      hs_hash[0]->init(hs_hash[0]->data);
      hs_hash[1]->init(hs_hash[1]->data);
#elif DTLS_VERSION == 0xfefd
      dtls_hash_init(hs_hash[0]);
#endif
    }
    
    if (packet[0] == 22 && data[0] == 2) { /* ServerHello */
      memcpy(dtls_kb_server_iv(OTHER_CONFIG), data + 14, 32);
      /* FIXME: search in ciphers */
      OTHER_CONFIG->cipher = TLS_PSK_WITH_AES_128_CCM_8;
    }
    
    if (packet[0] == 20 && data[0] == 1) { /* ChangeCipherSpec */
      printf("client random: ");
      dump(dtls_kb_client_iv(OTHER_CONFIG), 32);
      printf("\nserver random: ");
      dump(dtls_kb_server_iv(OTHER_CONFIG), 32);
      printf("\n");
      master_secret_len = 
	dtls_prf(pre_master_secret, pre_master_len,
		 (unsigned char *)"master secret", 13,
		 dtls_kb_client_iv(OTHER_CONFIG), 32,
		 dtls_kb_server_iv(OTHER_CONFIG), 32,
		 master_secret, DTLS_MASTER_SECRET_LENGTH);
  
      printf("master_secret:\n  ");
      for(i = 0; i < master_secret_len; i++) 
	printf("%02x", master_secret[i]);
      printf("\n");

      /* create key_block from master_secret
       * key_block = PRF(master_secret,
                     "key expansion" + server_random + client_random) */
      dtls_prf(master_secret, master_secret_len,
	       (unsigned char *)"key expansion", 13,
	       dtls_kb_server_iv(OTHER_CONFIG), 32,
	       dtls_kb_client_iv(OTHER_CONFIG), 32,
	       OTHER_CONFIG->key_block, 
	       dtls_kb_size(OTHER_CONFIG));

      OTHER_CONFIG->read_cipher = 
	dtls_cipher_new(OTHER_CONFIG->cipher,
			dtls_kb_client_write_key(OTHER_CONFIG),
			dtls_kb_key_size(OTHER_CONFIG));

      if (!OTHER_CONFIG->read_cipher) {
	warn("cannot create read cipher\n");
      } else {
	dtls_cipher_set_iv(OTHER_CONFIG->read_cipher,
			   dtls_kb_client_iv(OTHER_CONFIG),
			   dtls_kb_iv_size(OTHER_CONFIG));
      }

      OTHER_CONFIG->write_cipher = 
	dtls_cipher_new(OTHER_CONFIG->cipher, 
			dtls_kb_server_write_key(OTHER_CONFIG),
			dtls_kb_key_size(OTHER_CONFIG));
      
      if (!OTHER_CONFIG->write_cipher) {
	warn("cannot create write cipher\n");
      } else {
	dtls_cipher_set_iv(OTHER_CONFIG->write_cipher,
			   dtls_kb_server_iv(OTHER_CONFIG),
			   dtls_kb_iv_size(OTHER_CONFIG));
      }

      /* if (is_client) */
	SWITCH_CONFIG;
      epoch[is_client]++;

      printf("key_block:\n");
      printf("  client_MAC_secret:\t");  
      dump(dtls_kb_client_mac_secret(CURRENT_CONFIG), 
	   dtls_kb_mac_secret_size(CURRENT_CONFIG));
      printf("\n");

      printf("  server_MAC_secret:\t");  
      dump(dtls_kb_server_mac_secret(CURRENT_CONFIG), 
	   dtls_kb_mac_secret_size(CURRENT_CONFIG));
      printf("\n");

      printf("  client_write_key:\t");  
      dump(dtls_kb_client_write_key(CURRENT_CONFIG), 
	   dtls_kb_key_size(CURRENT_CONFIG));
      printf("\n");

      printf("  server_write_key:\t");  
      dump(dtls_kb_server_write_key(CURRENT_CONFIG), 
	   dtls_kb_key_size(CURRENT_CONFIG));
      printf("\n");

      printf("  client_IV:\t\t");  
      dump(dtls_kb_client_iv(CURRENT_CONFIG), 
	   dtls_kb_iv_size(CURRENT_CONFIG));
      printf("\n");
      
      printf("  server_IV:\t\t");  
      dump(dtls_kb_server_iv(CURRENT_CONFIG), 
	   dtls_kb_iv_size(CURRENT_CONFIG));
      printf("\n");
      
    }

    if (packet[0] == 22) {
      if (data[0] == 20) { /* Finished */
	finalize_hash(hash_buf);
	/* clear_hash(); */

	update_hash((unsigned char *)packet, sizeof(dtls_record_header_t),
		    data, data_length);

	dtls_prf(master_secret, master_secret_len,
		 is_client 
		 ? (unsigned char *)"client finished" 
		 : (unsigned char *)"server finished" 
		 , 15,
		 hash_buf, sizeof(hash_buf),
		 NULL, 0,
		 data + sizeof(dtls_handshake_header_t),
		 verify_data_length);
	printf("verify_data:\n");
	dump(data, data_length);
	printf("\n");
      } else {
	update_hash((unsigned char *)packet, sizeof(dtls_record_header_t),
		    data, data_length);
      }
    }

    if (packet[0] == 23) {	/* Application Data */
      printf("Application Data:\n");
      dump(data, data_length);
      printf("\n");
    }

  next:
    length -= rlen;
    packet += rlen;
  }
}

void init() {
  memset(security_params, 0, sizeof(security_params));
  CURRENT_CONFIG->cipher = -1;

  memset(hs_hash, 0, sizeof(hs_hash));

  /* set pre_master_secret to default if no PSK was given */
  if (!pre_master_len) {
    /* unsigned char psk[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }; */
    pre_master_len =
      dtls_pre_master_secret((unsigned char *)"secretPSK", 9,
    			     pre_master_secret);
  }
}

int main(int argc, char **argv) {
  pcap_t *pcap;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct pcap_pkthdr *pkthdr;
  const u_char *packet;
  int res = 0;
  int c, option_index = 0;

  static struct option opts[] = {
    { "psk",  1, 0, 'p' },
    { 0, 0, 0, 0 }
  };

  /* handle command line options */
  while (1) {
    c = getopt_long(argc, argv, "p:", opts, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'p':
      pre_master_len = dtls_pre_master_secret((unsigned char *)optarg, 
	      			      strlen(optarg), pre_master_secret);
      break;
    }
  }

  if (argc <= optind) {
    fprintf(stderr, "usage: %s [-p|--psk PSK] pcapfile\n", argv[0]);
    return -1;
  }

  init();

  pcap = pcap_open_offline(argv[optind], errbuf);
  if (!pcap) {
    fprintf(stderr, "pcap_open_offline: %s\n", errbuf);
    return -2;
  }

  for (;;) {
    res = pcap_next_ex(pcap, &pkthdr, &packet);
    
    switch(res) {
    case -2: goto done;
    case -1: pcap_perror(pcap, "read packet"); break;
    case  1: handle_packet(packet, pkthdr->caplen); break;
    default: 
      ;
    }      
  }
 done:

  pcap_close(pcap);

  return 0;
}
