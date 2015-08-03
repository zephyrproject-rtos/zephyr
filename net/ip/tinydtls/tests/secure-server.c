/* secure-server -- A (broken) DTLS server example
 *
 * Copyright (C) 2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#ifdef WITH_DTLS
#define SERVER_CERT_PEM "./server-cert.pem"
#define SERVER_KEY_PEM  "./server-key.pem"
#define CA_CERT_PEM     "./ca-cert.pem"
#endif

#ifdef HAVE_ASSERT_H
# include <assert.h>
#else
# define assert(x)
#endif /* HAVE_ASSERT_H */

static int quit=0;

/* SIGINT handler: set quit to 1 for graceful termination */
void
handle_sigint(int signum) {
  quit = 1;
}

int 
check_connect(int sockfd, char *buf, int buflen, 
	   struct sockaddr *src, int *ifindex) {

  /* for some reason, the definition in netinet/in.h is not exported */
#ifndef IN6_PKTINFO
  struct in6_pktinfo
  {
    struct in6_addr ipi6_addr;	/* src/dst IPv6 address */
    unsigned int ipi6_ifindex;	/* send/recv interface index */
  };
#endif

  size_t bytes;

  struct iovec iov[1] = { {buf, buflen} };
  char cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  struct in6_pktinfo *p = NULL;
  
  struct msghdr msg = { 0 };
  struct cmsghdr *cmsg;

  msg.msg_name = src;
  msg.msg_namelen = sizeof(struct sockaddr_in6);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  bytes = recvmsg(sockfd, &msg, MSG_DONTWAIT | MSG_PEEK);
  if (bytes < 0) {
    perror("recvmsg");
    return bytes;
  }

  /* TODO: handle msg.msg_flags & MSG_TRUNC */
  if (msg.msg_flags & MSG_CTRUNC) {
    fprintf(stderr, "control was truncated!\n");
    return -1;
  }

  if (ifindex) {
    /* Here we try to retrieve the interface index where the packet was received */
    *ifindex = 0;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	 cmsg = CMSG_NXTHDR(&msg, cmsg)) {

      if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
	p = (struct in6_pktinfo *)(CMSG_DATA(cmsg));
	*ifindex = p->ipi6_ifindex;
	break;
      }
    }
  }

  return bytes;
}

typedef enum { UNKNOWN=0, DTLS=1 } protocol_t;

protocol_t
demux_protocol(const char *buf, int len) {
  return DTLS;
}

#ifdef WITH_DTLS
typedef enum { 
  PEER_ST_ESTABLISHED, PEER_ST_PENDING, PEER_ST_CLOSED 
 } peer_state_t;
typedef struct {
  peer_state_t state;
  unsigned long h;
  SSL *ssl;
} ssl_peer_t;

#define MAX_SSL_PENDING      2	/* must be less than MAX_SSL_PEERS */
#define MAX_SSL_PEERS       10	/* MAX_SSL_PENDING of these might be pending  */
ssl_peer_t *ssl_peer_storage[MAX_SSL_PEERS];
static int pending = 0;

void
check_peers() {
typedef struct bio_dgram_data_st
	{
	union {
		struct sockaddr sa;
		struct sockaddr_in sa_in;
		struct sockaddr_in6 sa_in6;
	} peer;
	unsigned int connected;
	unsigned int _errno;
	unsigned int mtu;
	struct timeval next_timeout;
	struct timeval socket_timeout;
	} bio_dgram_data;

  struct sockaddr_in6 peer;
  int i;
  BIO *bio;
  for (i = 0; i < MAX_SSL_PEERS; i++) {
    if (ssl_peer_storage[i]) {
      if (!ssl_peer_storage[i]->ssl)
	fprintf(stderr, "invalid SSL object for peer %d!\n",i);
      else {
	bio = SSL_get_rbio(ssl_peer_storage[i]->ssl);
	if (bio) {
	  (void) BIO_dgram_get_peer(bio, (struct sockaddr *)&peer);
	  if (peer.sin6_port && ssl_peer_storage[i]->h != ntohs(peer.sin6_port)) {
	    fprintf(stderr, "   bio %p: port differs from hash: %d != %d! (%sconnected)\n", bio,
		    ssl_peer_storage[i]->h, 
		    ntohs(((struct sockaddr_in6 *)&peer)->sin6_port),
		    ((bio_dgram_data *)bio->ptr)->connected ? "" : "not ");
	  }

	}
      }
    }
  }
}

/** Creates a hash value from the first num bytes of s, taking init as
 * initialization value. */
static inline unsigned long
_hash(unsigned long init, const char *s, int num) {
  int c;

  while (num--)
    while ( (c = *s++) ) {
      init = ((init << 7) + init) + c;
    }

  return init;
}

static inline unsigned long
hash_peer(const struct sockaddr *peer, int ifindex) {
  unsigned long h;

  /* initialize hash value to interface index */
  h = _hash(0, (char *)&ifindex, sizeof(int));

#define CAST(TYPE,VAR) ((TYPE)VAR)

  assert(peer);
  switch (peer->sa_family) {
  case AF_INET: 
    return ntohs(CAST(const struct sockaddr_in *, peer)->sin_port);
    h = _hash(h, (char *) &CAST(const struct sockaddr_in *, peer)->sin_addr, 
	      sizeof(struct in_addr));
    h = _hash(h, (char *) &CAST(const struct sockaddr_in *, peer)->sin_port, 
	      sizeof(in_port_t));
    break;
  case AF_INET6:
    return ntohs(CAST(const struct sockaddr_in6 *, peer)->sin6_port);
    h = _hash(h, 
	      (char *) &CAST(const struct sockaddr_in6 *, peer)->sin6_addr, 
	      sizeof(struct in6_addr));
    h = _hash(h, 
	      (char *) &CAST(const struct sockaddr_in6 *, peer)->sin6_port, 
	      sizeof(in_port_t));
    break;
  default:
    /* last resort */
    h = _hash(h, (char *)peer, sizeof(struct sockaddr));
  }

  return 42;
  return h;
}

/* Returns index of peer object for specified address/ifindex pair. */
int
get_index_of_peer(const struct sockaddr *peer, int ifindex) {
  unsigned long h;
  int idx;
#ifndef NDEBUG
  char addr[INET6_ADDRSTRLEN];
  char port[6];
#endif

  if (!peer)
    return -1;

  h = hash_peer(peer,ifindex);

  for (idx = 0; idx < MAX_SSL_PEERS; idx++) {
    if (ssl_peer_storage[idx] && ssl_peer_storage[idx]->h == h) {
#ifndef NDEBUG
      getnameinfo((struct sockaddr *)peer, sizeof(struct sockaddr_in6), 
		  addr, sizeof(addr), port, sizeof(port), 
		  NI_NUMERICHOST | NI_NUMERICSERV);

      fprintf(stderr, "get_index_of_peer: [%s]:%s  =>  %lu\n",
	      addr, port, h);
#endif
      return idx;
    }
  }
  return -1;
}

SSL *
get_ssl(SSL_CTX *ctx, int sockfd, struct sockaddr *src, int ifindex) {
  int idx;
  BIO *bio;
  SSL *ssl;
#ifndef NDEBUG
  struct sockaddr_storage peer;
  char addr[INET6_ADDRSTRLEN];
  char port[6];
  int i;
#endif

  idx = get_index_of_peer(src,ifindex);
  if (idx >= 0) {
    fprintf(stderr,"found peer %d ",idx);
    switch (ssl_peer_storage[idx]->state) {
    case PEER_ST_ESTABLISHED: fprintf(stderr,"established\n"); break;
    case PEER_ST_PENDING:     fprintf(stderr,"pending\n"); break;
    case PEER_ST_CLOSED:      fprintf(stderr,"closed\n"); break;
    default:
      OPENSSL_assert(0);
    }

#ifndef NDEBUG
    memset(&peer, 0, sizeof(peer));
    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl_peer_storage[idx]->ssl), &peer);

    getnameinfo((struct sockaddr *)&peer, sizeof(peer), 
		addr, sizeof(addr), port, sizeof(port), 
		NI_NUMERICHOST | NI_NUMERICSERV);

    fprintf(stderr,"      [%s]:%s   \n", addr, port);
#endif
    return ssl_peer_storage[idx]->ssl;
  }

  /* none found, create new if sufficient space available */
  if (pending < MAX_SSL_PENDING) {
    for (idx = 0; idx < MAX_SSL_PEERS; idx++) {
      if (ssl_peer_storage[idx] == NULL) { /* found space */
	ssl = SSL_new(ctx);
	
	if (ssl) {
	  bio = BIO_new_dgram(sockfd, BIO_NOCLOSE);
	  if (!bio) {
	    SSL_free(ssl);
	    return NULL;
	  }
	  
	  SSL_set_bio(ssl, bio, bio);
	  SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);
	  
	  SSL_set_accept_state(ssl);
	  ssl_peer_storage[idx] = (ssl_peer_t *) malloc(sizeof(ssl_peer_t));
	  if (!ssl_peer_storage[idx]) {
	    SSL_free(ssl);
	    return NULL;
	  }
	  ssl_peer_storage[idx]->state = PEER_ST_PENDING;
	  ssl_peer_storage[idx]->h = hash_peer(src,ifindex);
	  ssl_peer_storage[idx]->ssl = ssl;
	  
	  pending++;
	  
	  fprintf(stderr,
		  "created new SSL peer %d for ssl object %p (storage: %p)\n", 
		 idx, ssl, ssl_peer_storage[idx]);
#ifndef NDEBUG
    if (getnameinfo((struct sockaddr *)&src, sizeof(src), 
		addr, sizeof(addr), port, sizeof(port), 
		    NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
      perror("getnameinfo");
      fprintf(stderr, "port was %u\n", ntohs(((struct sockaddr_in6 *)src)->sin6_port));
    } else {
    fprintf(stderr,"      [%s]:%s   \n", addr, port);
      }
#endif
    OPENSSL_assert(ssl_peer_storage[idx]->ssl == ssl);
	  fprintf(stderr,"%d objects pending\n", pending);
	  check_peers();
	  return ssl;
	}
      }
    }
  } else {
    fprintf(stderr, "too many pending SSL objects\n");
    return NULL;
  }

  fprintf(stderr, "too many peers\n");
  return NULL;
}

/** Deletes peer stored at index idx and frees allocated memory. */
static inline void
delete_peer(int idx) {
  if (idx < 0 || !ssl_peer_storage[idx])
    return;

  if (ssl_peer_storage[idx]->state == PEER_ST_PENDING)
    pending--;

  OPENSSL_assert(ssl_peer_storage[idx]->ssl);
  SSL_free(ssl_peer_storage[idx]->ssl);
    
  free(ssl_peer_storage[idx]);
  ssl_peer_storage[idx] = NULL;

  printf("deleted peer %d\n",idx);
}

/** Deletes all closed objects from ssl_peer_storage. */
void
remove_closed() {
  int idx;

  for (idx = 0; idx < MAX_SSL_PEERS; idx++)
    if (ssl_peer_storage[idx] 
	&& ssl_peer_storage[idx]->state == PEER_ST_CLOSED)
      delete_peer(idx);
}

#define min(a,b) ((a) < (b) ? (a) : (b))

unsigned int
psk_server_callback(SSL *ssl, const char *identity,
		    unsigned char *psk, unsigned int max_psk_len) {
  static char keybuf[] = "secretPSK";

  printf("psk_server_callback: check identity of client %s\n", identity);
  memcpy(psk, keybuf, min(strlen(keybuf), max_psk_len));

  return min(strlen(keybuf), max_psk_len);
}

#endif

#ifdef WITH_DTLS
/**
 * This function tracks the status changes from libssl to manage local
 * object state.
 */
void
info_callback(const SSL *ssl, int where, int ret) {
  int idx, i;
  struct sockaddr_storage peer;
  struct sockaddr_storage peer2;
  char addr[INET6_ADDRSTRLEN];
  char port[6];

  if (where & SSL_CB_LOOP)  /* do not care for intermediary states */
    return;

  memset(&peer, 0, sizeof(peer));
  (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

  /* lookup SSL object */   /* FIXME: need to get the ifindex */
  idx = get_index_of_peer((struct sockaddr *)&peer, 0);
  
  if (idx >= 0)
    fprintf(stderr, "info_callback: assert: %d < 0 || %p == %p (storage: %p)\n",
	    idx, ssl, ssl_peer_storage[idx]->ssl, ssl_peer_storage[idx]); 
  if (idx >= 0 && ssl != ssl_peer_storage[idx]->ssl) {
    getnameinfo((struct sockaddr *)&peer, sizeof(peer), 
		addr, sizeof(addr), port, sizeof(port), 
		NI_NUMERICHOST | NI_NUMERICSERV);

    fprintf(stderr," ssl: [%s]:%s   ", addr, port);
    
    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl_peer_storage[idx]->ssl), &peer2);
    getnameinfo((struct sockaddr *)&peer2, sizeof(peer2), 
		addr, sizeof(addr), port, sizeof(port), 
		NI_NUMERICHOST | NI_NUMERICSERV);

    fprintf(stderr," ssl_peer_storage[idx]->ssl: [%s]:%s\n", addr, port);

    fprintf(stderr, " hash:%lu     h: %lu\n",
	    hash_peer((const struct sockaddr *)&peer, 0),
	    ssl_peer_storage[idx]->h);

    for (i = 0; i < MAX_SSL_PEERS; i++) {
      if (ssl_peer_storage[i]) {
	fprintf(stderr, "%02d: %p ssl: %p  ",
		i, ssl_peer_storage[i] ,ssl_peer_storage[i]->ssl);

	(void) BIO_dgram_get_peer(SSL_get_rbio(ssl_peer_storage[i]->ssl), &peer2);
	getnameinfo((struct sockaddr *)&peer2, sizeof(peer2), 
		    addr, sizeof(addr), port, sizeof(port), 
		    NI_NUMERICHOST | NI_NUMERICSERV);
	
	fprintf(stderr," peer: [%s]:%s    h: %lu\n", addr, port, ssl_peer_storage[i]->h);
      }
    }
    fprintf(stderr, "***** ASSERT FAILED ******\n");

    memset(&peer, 0, sizeof(peer));
    (void) BIO_dgram_get_peer(SSL_get_wbio(ssl), &peer);

    idx = get_index_of_peer((struct sockaddr *)&peer, 0);
    fprintf(stderr, "  get_index_of_peer for wbio returns %d, type is %04x\n",
	    idx, where);    
  }
#if 1
	  check_peers();
  OPENSSL_assert((idx < 0) || (ssl == ssl_peer_storage[idx]->ssl));
#endif

  if (where & SSL_CB_ALERT) {
#ifndef NDEBUG
    if (ret != 0)
      fprintf(stderr,"%s:%s:%s\n", SSL_alert_type_string(ret),
	      SSL_alert_desc_string(ret), SSL_alert_desc_string_long(ret));
#endif

    /* examine alert type */
    switch (*SSL_alert_type_string(ret)) {
    case 'F':
      /* move SSL object from pending to close */
      if (idx >= 0) {
	ssl_peer_storage[idx]->state = PEER_ST_CLOSED;
	pending--;
      }
      break;
    case 'W': 
      if ((ret & 0xff) == SSL_AD_CLOSE_NOTIFY) {
	if (where == SSL_CB_WRITE_ALERT) 
	  fprintf(stderr,"sent CLOSE_NOTIFY\n");
	else /* received CN */
	  fprintf(stderr,"received CLOSE_NOTIFY\n");
      }
      break;
    default: 			/* handle unknown alert types */
#ifndef NDEBUG
      printf("not handled!\n");
#endif
    }
  }

  if (where & SSL_CB_HANDSHAKE_DONE) {
    /* move SSL object from pending to established */
    printf("HANDSHAKE_DONE ");
    if (idx >= 0) {
      
      if (ssl_peer_storage[idx]->state == PEER_ST_PENDING) {
	ssl_peer_storage[idx]->state = PEER_ST_ESTABLISHED;
	pending--;
	printf("moved SSL object %d to ESTABLISHED\n", idx);
	printf("%d objects pending\n", pending);
      } else {
#ifndef NDEBUG
	printf("huh, object %d was not pending? (%d)\n", idx,
	       ssl_peer_storage[idx]->state);
#endif
      }
      return;
    }
    return;
  }

  return;
}
#endif

#ifdef WITH_DTLS
/* checks if ssl object was closed and can be removed */
int 
check_close(SSL *ssl) {
  int res, err, idx;
  struct sockaddr_storage peer;
  
  memset(&peer, 0, sizeof(peer));
  (void)BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

  res = 0;
  if (SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) {
    printf("SSL_RECEIVED_SHUTDOWN\n");
    res = SSL_shutdown(ssl);
    if (res == 0) {
      printf("must call SSL_shutdown again\n");
      res = SSL_shutdown(ssl);
    }
    if (res < 0) {
	err = SSL_get_error(ssl,res);	
	fprintf(stderr, "shutdown: SSL error %d: %s\n", err,
		ERR_error_string(err, NULL));
    } 

    /* we can close the SSL object anyway */
    /* FIXME: need to get ifindex from somewhere */
    idx = get_index_of_peer((struct sockaddr *)&peer, 0);
    OPENSSL_assert(idx < 0 || ssl == ssl_peer_storage[idx]->ssl);
    if (idx >= 0) {
      ssl_peer_storage[idx]->state = PEER_ST_CLOSED;
      printf("moved SSL object %d to CLOSED\n",idx);
    }
  }
  
  return res;
}

int 
check_timeout() {
  int i, result, err;

  for (i = 0; i < MAX_SSL_PEERS; i++) {
    if (ssl_peer_storage[i]) {
      OPENSSL_assert(ssl_peer_storage[i]->ssl);
      result = DTLSv1_handle_timeout(ssl_peer_storage[i]->ssl);
      if (result < 0) {
	err = SSL_get_error(ssl_peer_storage[i]->ssl,result);
	fprintf(stderr, "dtls1_handle_timeout (%d): %s\n",
		err, ERR_error_string(err, NULL));
      }
    }
  }

  /* remove outdated obbjects? */
  
  return 0;
}
#endif /* WITH_DTLS */
  
int 
_read(SSL_CTX *ctx, int sockfd) {
  char buf[2000];
  struct sockaddr_in6 src;
  int len, ifindex, i;
  char addr[INET6_ADDRSTRLEN];
  char port[6];
  socklen_t sz = sizeof(struct sockaddr_in6);
#ifdef WITH_DTLS
  SSL *ssl;
  int err;
#endif

  /* Retrieve remote address and interface index as well as the first
     few bytes of the message to demultiplex protocols. */
  memset(&src, 0, sizeof(struct sockaddr_in6));
  len = check_connect(sockfd, buf, 4, (struct sockaddr *)&src, &ifindex);

  if (len < 0)			/* error */
    return len;

#ifndef NDEBUG
  fprintf(stderr,"received packet");
  
  if (getnameinfo((struct sockaddr *)&src, sizeof(src), 
		  addr, sizeof(addr), port, sizeof(port), 
		  NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    fprintf(stderr," from [%s]:%s", addr, port);
  
  fprintf(stderr," on interface %d\n", ifindex);
#endif

  switch (demux_protocol(buf, len)) {
#ifdef WITH_DTLS
  case DTLS :
    ssl = get_ssl(ctx, sockfd, (struct sockaddr *)&src, ifindex);
    if (!ssl) {
      fprintf(stderr, "cannot create new SSL object\n");
      /*      return recv(sockfd, buf, sizeof(buf), MSG_DONTWAIT);*/
      len = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT,
		     (struct sockaddr *)&src, &sz);
      getnameinfo((struct sockaddr *)&src, sz, 
		  addr, sizeof(addr), port, sizeof(port), 
		  NI_NUMERICHOST | NI_NUMERICSERV);
      printf("discarded %d bytes from [%s]:%s\n", len, addr, port);      
      return len;
    }
    len = SSL_read(ssl, buf, sizeof(buf));
    break;
#endif
  case UNKNOWN:
  default :
    len = recv(sockfd, buf, sizeof(buf), MSG_DONTWAIT);
  }

  if (len > 0) {
    printf("here is the data:\n");
    for (i=0; i<len; i++)
      printf("%c",buf[i]);
  } if (len == 0) {		/* session closed? */
#ifdef WITH_DTLS
    if (check_close(ssl) <= 0) {
      fprintf(stderr, "not closed\n");
    }
#endif
  } else {
#ifdef WITH_DTLS
    err = SSL_get_error(ssl,len);
    switch (err) {
    case SSL_ERROR_WANT_READ:
      fprintf(stderr, "SSL_ERROR_WANT_READ\n");
      return 0;
    case SSL_ERROR_WANT_WRITE:
      fprintf(stderr, "SSL_ERROR_WANT_WRITE\n");
      return 0;
    default:
      fprintf(stderr, "read: SSL error %d: %s\n", err,
	      ERR_error_string(err, NULL));
      return 0;
    }
#else
    perror("recv");
#endif
  }

  return len;
}

int 
_write(SSL_CTX *ctx, int sockfd) {
  int res = 0;
#ifdef WITH_DTLS
  SSL *ssl;
  int err;

  ssl = get_ssl(ctx, sockfd, NULL, 1);
  if (!ssl) {
    fprintf(stderr, "no SSL object for writing");
    return 0;
  }
  res = SSL_write(ssl, NULL, 0);
  if (res < 0) {
    /*
    if (SSL_want_write(ssl))
      return 0;
    */
    /* FIXME: check SSL_want_read(ssl) */

    err = SSL_get_error(ssl,res);
    fprintf(stderr,"SSL_write returned %d (%s)\n", err, ERR_error_string(err, NULL));
  } else {
    printf("SSL_write successful\n");
  }
#else
#endif
  
  return res;
}


int 
generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {
  /* FIXME: generate secure client-specific cookie */
#define DUMMYSTR "ABCDEFGHIJKLMNOP"
  *cookie_len = strlen(DUMMYSTR);
  memcpy(cookie, DUMMYSTR, *cookie_len);

  return 1;
}

int 
verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len) {
  /* FIXME */
  return 1;
}

enum { READ, WRITE };

int
main(int argc, char **argv) {
  int sockfd = 0;
  int on = 1;
  struct sockaddr_in6 listen_addr = { AF_INET6, htons(20220), 0, IN6ADDR_ANY_INIT, 0 };
  size_t addr_size = sizeof(struct sockaddr_in6);
  fd_set fds[2];
  int result, flags;
  int idx, res = 0;
  struct timeval timeout;
  struct sigaction act, oact;
  
#ifdef WITH_DTLS
  SSL_CTX *ctx;

  memset(ssl_peer_storage, 0, sizeof(ssl_peer_storage));

  SSL_load_error_strings();
  SSL_library_init();
  ctx = SSL_CTX_new(DTLSv1_server_method());

  SSL_CTX_set_cipher_list(ctx, "ALL");
  SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

  res = SSL_CTX_use_certificate_file(ctx, SERVER_CERT_PEM, SSL_FILETYPE_PEM);
  if (res != 1) {
    fprintf(stderr, "cannot read server certificate from file '%s' (%s)\n", 
	    SERVER_CERT_PEM, ERR_error_string(res,NULL));
    goto end;
  }

  res = SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY_PEM, SSL_FILETYPE_PEM);
  if (res != 1) {
    fprintf(stderr, "cannot read server key from file '%s' (%s)\n", 
	    SERVER_KEY_PEM, ERR_error_string(res,NULL));
    goto end;
  }

  res = SSL_CTX_check_private_key (ctx);
  if (res != 1) {
    fprintf(stderr, "invalid private key\n");
    goto end;
  }

  res = SSL_CTX_load_verify_locations(ctx, CA_CERT_PEM, NULL);
  if (res != 1) {
    fprintf(stderr, "cannot read ca file '%s'\n", CA_CERT_PEM);
    goto end;
  }

  /* Client has to authenticate */

  /* Client has to authenticate */
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);

  SSL_CTX_set_read_ahead(ctx, 1); /* disable read-ahead */
  SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie);
  SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie);

  SSL_CTX_use_psk_identity_hint(ctx, "Enter password for CoAP-Gateway");
  SSL_CTX_set_psk_server_callback(ctx, psk_server_callback);

  SSL_CTX_set_info_callback(ctx, info_callback);
#endif

  sockfd = socket(listen_addr.sin6_family, SOCK_DGRAM, 0);
  if ( sockfd < 0 ) {
    perror("socket");
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0)
    perror("setsockopt SO_REUSEADDR");

  flags = fcntl(sockfd, F_GETFL, 0);
  if (flags < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror("fcntl");
    return -1;
  }

  on = 1;
  if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on) ) < 0) {
    perror("setsockopt IPV6_PKTINFO");
  }

  if (bind (sockfd, (const struct sockaddr *)&listen_addr, addr_size) < 0) {
    perror("bind");
    res = -2;
    goto end;
  }

  act.sa_handler = handle_sigint;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, &oact);

  while (!quit) {
    FD_ZERO(&fds[READ]);
    FD_ZERO(&fds[WRITE]);
    FD_SET(sockfd, &fds[READ]);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    result = select( FD_SETSIZE, &fds[READ], &fds[WRITE], 0, &timeout);

    if (result < 0) {		/* error */
      if (errno != EINTR)
	perror("select");
    } else if (result > 0) {	/* read from socket */
      if ( FD_ISSET( sockfd, &fds[READ]) ) {
	_read(ctx, sockfd);	/* read received data */
      } else if ( FD_ISSET( sockfd, &fds[WRITE]) ) { /* write to socket */
	_write(ctx, sockfd);		/* write data */
      }
    } else {			/* timeout */
      check_timeout();
    }
    remove_closed();
  }
  
 end:
#ifdef WITH_DTLS
  for (idx = 0; idx < MAX_SSL_PEERS; idx++) {
    if (ssl_peer_storage[idx] && ssl_peer_storage[idx]->ssl) {
      if (ssl_peer_storage[idx]->state == PEER_ST_ESTABLISHED)
	SSL_shutdown(ssl_peer_storage[idx]->ssl);
      SSL_free(ssl_peer_storage[idx]->ssl);
    }
  }

  SSL_CTX_free(ctx);
#endif
  close(sockfd);		/* don't care if we close stdin at this point */
  return res;
}
