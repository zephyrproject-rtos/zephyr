#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "dsrv.h" 

void
handle_read(struct dsrv_context_t *ctx) {
  int len;
  static char buf[200];
  struct sockaddr_storage src;
  socklen_t srclen = sizeof(src);
  int fd = dsrv_get_fd(ctx, DSRV_READ);

  len = recvfrom(fd, buf, sizeof(buf), 0, 
		 (struct sockaddr *)&src, &srclen);

  if (len < 0) {
    perror("recvfrom");
  } else {
    printf("read %d bytes: '%*s'\n", len, len, buf);
    if (dsrv_sendto(ctx, (struct sockaddr *)&src, srclen, 0, buf, len) 
	== NULL) {
      fprintf(stderr, "cannot add packet to sendqueue\n");
    }
  }
}

int
handle_write(struct dsrv_context_t *ctx) {
  struct packet_t *p;
  int fd = dsrv_get_fd(ctx, DSRV_WRITE);
  int len;

  p = ctx->rq ? nq_peek(ctx->wq) : NULL;

  if (!p)
    return -1;

  len = sendto(fd, p->buf, p->len, 0, p->raddr, p->rlen);
  
  if (len < 0)
    perror("sendto");
  else 
    nq_pop(ctx->wq);

  return len;
}

int main(int argc, char **argv) {

#if 1
  struct sockaddr_in6 listen_addr = { AF_INET6, htons(20220), 0, IN6ADDR_ANY_INIT, 0 };
#else
  struct sockaddr_in listen_addr = { AF_INET, htons(20220), { htonl(0x7f000001) } };
#endif
  fd_set rfds, wfds;
  struct timeval timeout;
  struct dsrv_context_t *ctx;
  int result;

  ctx = dsrv_new_context((struct sockaddr *)&listen_addr, sizeof(listen_addr), 
			 200,200);

  if (!ctx) {
    fprintf(stderr, "E: cannot create server context\n");
    return -1;
  }

  while (1) {
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    dsrv_prepare(ctx, &rfds, DSRV_READ);
    dsrv_prepare(ctx, &wfds, DSRV_WRITE);
    
#if 0
    timeout.tv_sec = 0;
    timeout.tv_usec = dsrv_get_timeout(ctx);
#else
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
#endif
    
    result = select( FD_SETSIZE, &rfds, &wfds, 0, &timeout);
    
    if (result < 0) {		/* error */
      if (errno != EINTR)
	perror("select");
    } else if (result == 0) {	/* timeout */
      printf(".");		
    } else {			/* ok */
      if (dsrv_check(ctx, &wfds, DSRV_WRITE))
	handle_write(ctx);
      else if (dsrv_check(ctx, &rfds, DSRV_READ))
	handle_read(ctx);
    }
  }

  dsrv_close(ctx);
  dsrv_free_context(ctx);

  return 0;
}
