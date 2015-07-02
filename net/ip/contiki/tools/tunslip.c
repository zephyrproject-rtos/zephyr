/*
 * Copyright (c) 2001, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>

int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));
void write_to_serial(int outfd, void *inbuf, int len);

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while (0)

struct ip {
  u_int8_t ip_vhl;		/* version and header length */
#define IP_V4 0x40
#define IP_V  0xf0
#define IP_HL 0x0f
  u_int8_t ip_tos;                    /* type of service */
  u_int16_t ip_len;                     /* total length */
  u_int16_t ip_id;                      /* identification */
  u_int16_t ip_off;                     /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
  u_int8_t ip_ttl;                    /* time to live */
  u_int8_t ip_p;                      /* protocol */
  u_int16_t ip_sum;                     /* checksum */
  u_int32_t ip_src, ip_dst;      /* source and dest address */
  u_int16_t uh_sport;		/* source port */
  u_int16_t uh_dport;		/* destination port */
  u_int16_t uh_ulen;		/* udp length */
  u_int16_t uh_sum;		/* udp checksum */
};

int check_ip(const struct ip *ip, unsigned ip_len);
u_int16_t ip4sum(u_int16_t sum, const void *_p, u_int16_t len);

struct dhcp_msg {
  u_int8_t op, htype, hlen, hops;
  u_int8_t xid[4];
  u_int16_t secs, flags;
  u_int8_t ciaddr[4];
  u_int8_t yiaddr[4];
  u_int8_t siaddr[4];
  u_int8_t giaddr[4];
  u_int8_t chaddr[16];
#define DHCP_BASE_LEN (4*7 + 16)
  u_int8_t sname[64];
  u_int8_t file[128];
#define DHCP_HOLE_LEN (64 + 128)
#define DHCP_MSG_LEN (DHCP_BASE_LEN + DHCP_HOLE_LEN)
  u_int8_t options[312];
};

struct dhcp_light_msg {
  u_int8_t op, htype, hlen, hops;
  u_int8_t xid[4];
  u_int16_t secs, flags;
  u_int8_t ciaddr[4];
  u_int8_t yiaddr[4];
  u_int8_t siaddr[4];
  u_int8_t giaddr[4];
  u_int8_t chaddr[16];
#define DHCP_LIGHT_MSG_LEN (4*7 + 16)
  u_int8_t options[312];
};

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_AGENT       82
#define DHCP_OPTION_SUBNET_SELECTION 118
#define DHCP_OPTION_END         255

/* DHCP_OPTION_AGENT, Relay Agent Information option subtypes: */
#define RAI_CIRCUIT_ID  1
#define RAI_REMOTE_ID   2
#define RAI_AGENT_ID    3
#define RAI_SUBNET_SELECTION    5

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define BOOTP_BROADCAST 0x8000

#define BOOTPS 67
#define BOOTPC 68

#define BOOTREQUEST 1
#define BOOTREPLY   2

in_addr_t giaddr;
in_addr_t netaddr;
in_addr_t circuit_addr;

char tundev[1024] = { "tun0" };

struct sockaddr_in dhaddr;
int dhsock = -1;

void
relay_dhcp_to_server(struct ip *ip, int len)
{
  struct dhcp_light_msg *inm;
  struct dhcp_msg m;
  int n;
  u_int8_t *optptr;

  inm = (void*)(((u_int8_t*)ip) + 20 + 8); /* Skip over IP&UDP headers. */

  if (inm->op != BOOTREQUEST) {
    return;
  }

  inm->flags = ntohs(BOOTP_BROADCAST);

  memcpy(&m, inm, DHCP_BASE_LEN);
  memset(&m.sname, 0x0, DHCP_HOLE_LEN);
  memcpy(&m.options, &inm->options, len - 20 - 8 - DHCP_BASE_LEN);
  n = (len - 20 - 8) + DHCP_HOLE_LEN; /* +HOLE -IP&UDP headers. */

  /*
   * Ideally we would like to use the Relay Agent information option
   * (RFC3046) together with the Link Selection sub-option (RFC3527)
   * to ensure that addresses are allocated for this
   * subnet. Unfortunately ISC-DHCPD does not currently implement
   * RFC3527 and some other mechanism must be used. For this reason
   * this implementation in addition uses the DHCP option for subnet
   * selection (RFC3011) which is really not intended to be used by
   * relays.
   *
   * Find DHCP_OPTION_END and add the new option here.
   */
  optptr = &m.options[n - DHCP_BASE_LEN - DHCP_HOLE_LEN - 1];
  {
    *optptr++ = DHCP_OPTION_SUBNET_SELECTION; /* RFC3011 */
    *optptr++ = 4;
    memcpy(optptr, &netaddr, 4); optptr += 4;
    n += 4 + 2;
  }
  {
    *optptr++ = DHCP_OPTION_AGENT; /* RFC3046 */
    *optptr++ = 18; /* Sum of all suboptions below! */

    *optptr++ = RAI_SUBNET_SELECTION; /* RFC3527 */
    *optptr++ = 4;
    memcpy(optptr, &netaddr, 4); optptr += 4;
    *optptr++ = RAI_CIRCUIT_ID;
    *optptr++ = 4;
    memcpy(optptr, &circuit_addr, 4); optptr += 4;
    *optptr++ = RAI_AGENT_ID;
    *optptr++ = 4;
    memcpy(optptr, &giaddr, 4); optptr += 4;
    n += 18 + 2;			/* Sum of all suboptions + 2! */
  }
  /* And finally put back the END. */
  *optptr++ = DHCP_OPTION_END;

  m.hops++;
  memcpy(m.giaddr, &giaddr, sizeof(m.giaddr));
  if (n != sendto(dhsock, &m, n, 0x0/*flags*/,
		(struct sockaddr *)&dhaddr, sizeof(dhaddr)))
    err(1, "sendto relay failed");
}

static u_int16_t ip_id;

void
relay_dhcp_to_client(int slipfd)
{
  struct dhcp_msg inm;
  struct {
    struct ip ip;
    struct dhcp_light_msg m;
  } pkt;
  int n, optlen, ip_len, udp_len;
  u_int8_t *p, *t, *end;
  u_int16_t sum;
  u_int8_t op, msg_type = 0;
  struct in_addr yiaddr;

  memset(&inm.options, 0x0, sizeof(inm.options));

  n = recv(dhsock, &inm, sizeof(inm), 0x0/*flags*/);

  if (inm.op != BOOTREPLY) {
    return;
  }

  memcpy(&yiaddr, inm.yiaddr, sizeof(inm.yiaddr));
  memcpy(&pkt.m, &inm, DHCP_BASE_LEN);
  pkt.m.hops++;
  memset(pkt.m.giaddr, 0x0, sizeof(pkt.m.giaddr));

  /*
   * Copy options we would like to send to client.
   */
  memcpy(pkt.m.options, inm.options, 4); /* Magic cookie */

  end = &inm.op + n;
  p = inm.options + 4;		/* Magic cookie */
  t = pkt.m.options + 4;	/* Magic cookie */
  while (p < end) {
    op = p[0];
    switch (op) {
    case DHCP_OPTION_END:
      goto done;

    case DHCP_OPTION_MSG_TYPE:
      msg_type = p[2];
    case DHCP_OPTION_SUBNET_MASK:
    case DHCP_OPTION_ROUTER:
    case DHCP_OPTION_LEASE_TIME:
    case DHCP_OPTION_SERVER_ID:	/* Copy these options */
      memcpy(t, p, p[1] + 2);
      t += p[1] + 2;
      p += p[1] + 2;
      break;

    case DHCP_OPTION_DNS_SERVER: /* Only copy first server */
      *t++ = p[0];
      *t++ = 4;
      memcpy(t, p + 2, 4); t += 4;
      p += p[1] + 2;
      break;

    default:			/* Ignore these options */
      /* printf("option type %d len %d\n", op, p[1]); */
      p += p[1] + 2;
      continue;
    }
  }
 done:
  if (op == DHCP_OPTION_END) {
    *t++ = op; *p++;
  }

  optlen = t - pkt.m.options;
  ip_len = 20 + 8 + DHCP_BASE_LEN + optlen;
  udp_len = 8 + DHCP_BASE_LEN + optlen;

  pkt.ip.ip_vhl = 0x45;		/* IPv4 and hdrlen=5*4 */
  pkt.ip.ip_tos = 0;
  pkt.ip.ip_len = htons(ip_len);
  pkt.ip.ip_id = htons(ip_id++);
  pkt.ip.ip_off = 0;
  pkt.ip.ip_ttl = 64;
  pkt.ip.ip_p = 17;		/* proto UDP */
  pkt.ip.ip_sum = 0;
  pkt.ip.ip_src = giaddr;
  if (inm.flags & htons(BOOTP_BROADCAST)) /* check bcast bit */
    pkt.ip.ip_dst = 0xffffffff;	/* 255.255.255.255 */
  else
    pkt.ip.ip_dst = yiaddr.s_addr;

  pkt.ip.uh_sport = htons(BOOTPS);
  pkt.ip.uh_dport = htons(BOOTPC);
  pkt.ip.uh_ulen = htons(udp_len);
  pkt.ip.uh_sum = 0;

  pkt.ip.ip_sum = ~htons(ip4sum(0, &pkt.ip, 20));
  sum = 17 + udp_len;
  sum = ip4sum(sum, &pkt.ip.ip_src, 8);
  sum = ip4sum(sum, &pkt.ip.uh_sport, udp_len);
  if (sum != 0xffff)
    pkt.ip.uh_sum = ~htons(sum);
  else
    pkt.ip.uh_sum = 0xffff;

  write_to_serial(slipfd, &pkt, ip_len);
  if (msg_type == DHCPACK) {
    printf("DHCPACK %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x IP %s\n",
	   pkt.m.chaddr[0], pkt.m.chaddr[1], pkt.m.chaddr[2], pkt.m.chaddr[3],
	   pkt.m.chaddr[4], pkt.m.chaddr[5], pkt.m.chaddr[6], pkt.m.chaddr[7],
	   inet_ntoa(yiaddr));
    /* ssystem("arp -s %s auto pub only", inet_ntoa(yiaddr)); */
  }
}

/*
 * Internet checksum in host byte order.
 */
u_int16_t
ip4sum(u_int16_t sum, const void *_p, u_int16_t len)
{
  u_int16_t t;
  const u_int8_t *p = _p;
  const u_int8_t *end = p + len;

  while(p < (end-1)) {
    t = (p[0] << 8) + p[1];
    sum += t;
    if (sum < t) sum++;
    p += 2;
  }
  if(p < end) {
    t = (p[0] << 8) + 0;
    sum += t;
    if (sum < t) sum++;
  }
  return sum;
}

int
check_ip(const struct ip *ip, unsigned ip_len)
{
  u_int16_t sum, ip_hl;

  /* Check IP version and length. */
  if((ip->ip_vhl & IP_V) != IP_V4)
    return -1;

  if(ntohs(ip->ip_len) > ip_len)
    return -2;

  if(ntohs(ip->ip_len) < ip_len)
    return -3;

  /* Check IP header. */
  ip_hl = 4*(ip->ip_vhl & IP_HL);
  sum = ip4sum(0, ip, ip_hl);
  if(sum != 0xffff && sum != 0x0)
    return -4;

  if(ip->ip_p == 6 || ip->ip_p == 17) {	/* Check TCP or UDP header. */
    u_int16_t tcp_len = ip_len - ip_hl;

    /* Sum pseudoheader. */
    sum = ip->ip_p + tcp_len; /* proto and len, no carry */
    sum = ip4sum(sum, &ip->ip_src, 8); /* src and dst */

    /* Sum TCP/UDP header and data. */
    sum = ip4sum(sum, (u_int8_t*)ip + ip_hl, tcp_len);

    /* Failed checksum test? */
    if (sum != 0xffff && sum != 0x0) {
      if (ip->ip_p == 6) {	/* TCP == 6 */
	return -5;
      } else {			/* UDP */
	/* Deal with disabled UDP checksums. */
	if (ip->uh_sum != 0)
	  return -6;
      }
    }
  } else if (ip->ip_p == 1) {	/* ICMP */
    u_int16_t icmp_len = ip_len - ip_hl;

    sum = ip4sum(0, (u_int8_t*)ip + ip_hl, icmp_len);
    if(sum != 0xffff && sum != 0x0)
      return -7;
  }
  return 0;
}

int
is_sensible_string(const unsigned char *s, int len)
{
  int i;
  for(i = 1; i < len; i++) {
    if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
      continue;
    } else if(s[i] < ' ' || '~' < s[i]) {
      return 0;
    }
  }
  return 1;
}

int
ssystem(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

int
ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  printf("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/*
 * Read from serial, when we have a packet write it to tun. No output
 * buffering, input buffered by stdio.
 */
void
serial_to_tun(FILE *inslip, int outfd)
{
  static union {
    unsigned char inbuf[2000];
    struct ip iphdr;
  } uip;
  static int inbufptr = 0;

  int ret;
  unsigned char c;

#ifdef linux
  ret = fread(&c, 1, 1, inslip);
  if(ret == -1 || ret == 0) err(1, "serial_to_tun: read");
  goto after_fread;
#endif

 read_more:
  if(inbufptr >= sizeof(uip.inbuf)) {
     inbufptr = 0;
  }
  ret = fread(&c, 1, 1, inslip);
#ifdef linux
 after_fread:
#endif
  if(ret == -1) {
    err(1, "serial_to_tun: read");
  }
  if(ret == 0) {
    clearerr(inslip);
    return;
    fprintf(stderr, "serial_to_tun: EOF\n");
    exit(1);
  }
  /*  fprintf(stderr, ".");*/
  switch(c) {
  case SLIP_END:
    if(inbufptr > 0) {
      /*
       * Sanity checks.
       */
#define DEBUG_LINE_MARKER '\r'
      int ecode;
      ecode = check_ip(&uip.iphdr, inbufptr);
      if(ecode < 0 && inbufptr == 8 && strncmp(uip.inbuf, "=IPA", 4) == 0) {
	static struct in_addr ipa;

	inbufptr = 0;
	if(memcmp(&ipa, &uip.inbuf[4], sizeof(ipa)) == 0) {
	  break;
	}

	/* New address. */
	if(ipa.s_addr != 0) {
#ifdef linux
	  ssystem("route delete -net %s netmask %s dev %s",
		  inet_ntoa(ipa), "255.255.255.255", tundev);
#else
	  ssystem("route delete -net %s -netmask %s -interface %s",
		  inet_ntoa(ipa), "255.255.255.255", tundev);
#endif
	}

	memcpy(&ipa, &uip.inbuf[4], sizeof(ipa));
	if(ipa.s_addr != 0) {
#ifdef linux
	  ssystem("route add -net %s netmask %s dev %s",
		  inet_ntoa(ipa), "255.255.255.255", tundev);
#else
	  ssystem("route add -net %s -netmask %s -interface %s",
		  inet_ntoa(ipa), "255.255.255.255", tundev);
#endif
	}
	break;
      } else if(ecode < 0) {
	/*
	 * If sensible ASCII string, print it as debug info!
	 */
	if(uip.inbuf[0] == DEBUG_LINE_MARKER) {
	  fwrite(uip.inbuf + 1, inbufptr - 1, 1, stderr);
	} else if(is_sensible_string(uip.inbuf, inbufptr)) {
	  fwrite(uip.inbuf, inbufptr, 1, stderr);
	} else {
	  fprintf(stderr,
		  "serial_to_tun: drop packet len=%d ecode=%d\n",
		  inbufptr, ecode);
	}
	inbufptr = 0;
	break;
      }
      PROGRESS("s");

      if(dhsock != -1) {
	struct ip *ip = (void *)uip.inbuf;
	if(ip->ip_p == 17 && ip->ip_dst == 0xffffffff /* UDP and broadcast */
	    && ip->uh_sport == ntohs(BOOTPC) && ip->uh_dport == ntohs(BOOTPS)) {
	  relay_dhcp_to_server(ip, inbufptr);
	  inbufptr = 0;
	}
      }
      if(write(outfd, uip.inbuf, inbufptr) != inbufptr) {
	err(1, "serial_to_tun: write");
      }
      inbufptr = 0;
    }
    break;

  case SLIP_ESC:
    if(fread(&c, 1, 1, inslip) != 1) {
      clearerr(inslip);
      /* Put ESC back and give up! */
      ungetc(SLIP_ESC, inslip);
      return;
    }

    switch(c) {
    case SLIP_ESC_END:
      c = SLIP_END;
      break;
    case SLIP_ESC_ESC:
      c = SLIP_ESC;
      break;
    }
    /* FALLTHROUGH */
  default:
    uip.inbuf[inbufptr++] = c;
    break;
  }

  goto read_more;
}

unsigned char slip_buf[2000];
int slip_end, slip_begin;

void
slip_send(int fd, unsigned char c)
{
  if (slip_end >= sizeof(slip_buf))
    err(1, "slip_send overflow");
  slip_buf[slip_end] = c;
  slip_end++;
}

int
slip_empty()
{
  return slip_end == 0;
}

void
slip_flushbuf(int fd)
{
  int n;

  if (slip_empty())
    return;

  n = write(fd, slip_buf + slip_begin, (slip_end - slip_begin));

  if(n == -1 && errno != EAGAIN) {
    err(1, "slip_flushbuf write failed");
  } else if(n == -1) {
    PROGRESS("Q");		/* Outqueueis full! */
  } else {
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
    }
  }
}

void
write_to_serial(int outfd, void *inbuf, int len)
{
  u_int8_t *p = inbuf;
  int i, ecode;
  struct ip *iphdr = inbuf;

  /*
   * Sanity checks.
   */
  ecode = check_ip(inbuf, len);
  if(ecode < 0) {
    fprintf(stderr, "tun_to_serial: drop packet %d\n", ecode);
    return;
  }

  if(iphdr->ip_id == 0 && iphdr->ip_off & IP_DF) {
    uint16_t nid = htons(ip_id++);
    iphdr->ip_id = nid;
    nid = ~nid;			/* negate */
    iphdr->ip_sum += nid;	/* add */
    if(iphdr->ip_sum < nid) {	/* 1-complement overflow? */
      iphdr->ip_sum++;
    }
    ecode = check_ip(inbuf, len);
    if(ecode < 0) {
      fprintf(stderr, "tun_to_serial: drop packet %d\n", ecode);
      return;
    }
  }

  /* It would be ``nice'' to send a SLIP_END here but it's not
   * really necessary.
   */
  /* slip_send(outfd, SLIP_END); */

  for(i = 0; i < len; i++) {
    switch(p[i]) {
    case SLIP_END:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_END);
      break;
    case SLIP_ESC:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_ESC);
      break;
    default:
      slip_send(outfd, p[i]);
      break;
    }

  }
  slip_send(outfd, SLIP_END);
  PROGRESS("t");
}


/*
 * Read from tun, write to slip.
 */
void
tun_to_serial(int infd, int outfd)
{
  static union {
    unsigned char inbuf[2000];
    struct ip iphdr;
  } uip;
  int size;

  if((size = read(infd, uip.inbuf, 2000)) == -1) err(1, "tun_to_serial: read");

  write_to_serial(outfd, uip.inbuf, size);
}

#ifndef BAUDRATE
#define BAUDRATE B115200
#endif
speed_t b_rate = BAUDRATE;

void
stty_telos(int fd)
{
  struct termios tty;
  speed_t speed = b_rate;
  int i;

  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");

  if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

  cfmakeraw(&tty);

  /* Nonblocking read. */
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag &= ~HUPCL;
  tty.c_cflag &= ~CLOCAL;

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

#if 0
  /* Nonblocking read and write. */
  /* if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) err(1, "fcntl"); */

  tty.c_cflag |= CLOCAL;
  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

  i = TIOCM_DTR;
  if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

  usleep(10*1000);		/* Wait for hardware 10ms. */

  /* Flush input and output buffers. */
  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}

int
devopen(const char *dev, int flags)
{
  char t[1024];
  strcpy(t, "/dev/");
  strcat(t, dev);
  return open(t, flags);
}

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>

int
tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
    return -1;

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if(*dev != 0)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}
#else
int
tun_alloc(char *dev)
{
  return devopen(dev, O_RDWR);
}
#endif

const char *ipaddr;
const char *netmask;

void
cleanup(void)
{
  ssystem("ifconfig %s down", tundev);
#ifndef linux
  ssystem("sysctl -w net.inet.ip.forwarding=0");
#endif
  /* ssystem("arp -d %s", ipaddr); */
  ssystem("netstat -nr"
	  " | awk '{ if ($2 == \"%s\") print \"route delete -net \"$1; }'"
	  " | sh",
	  tundev);
}

void
sigcleanup(int signo)
{
  fprintf(stderr, "signal %d\n", signo);
  exit(0);			/* exit(0) will call cleanup() */
}

static int got_sigalarm;

void
sigalarm(int signo)
{
  got_sigalarm = 1;
  return;
}

void
sigalarm_reset()
{
#ifdef linux
#define TIMEOUT (997*1000)
#else
#define TIMEOUT (2451*1000)
#endif
  ualarm(TIMEOUT, TIMEOUT);
  got_sigalarm = 0;
}

void
ifconf(const char *tundev, const char *ipaddr, const char *netmask)
{
  struct in_addr netname;
  netname.s_addr = inet_addr(ipaddr) & inet_addr(netmask);

#ifdef linux
  ssystem("ifconfig %s inet `hostname` up", tundev);
  if(strcmp(ipaddr, "0.0.0.0") != 0) {
    ssystem("route add -net %s netmask %s dev %s",
	    inet_ntoa(netname), netmask, tundev);
  }
#else
  ssystem("ifconfig %s inet `hostname` %s up", tundev, ipaddr);
  if(strcmp(ipaddr, "0.0.0.0") != 0) {
    ssystem("route add -net %s -netmask %s -interface %s",
	    inet_ntoa(netname), netmask, tundev);
  }
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  ssystem("ifconfig %s\n", tundev);
}

int
main(int argc, char **argv)
{
  int c;
  int tunfd, slipfd, maxfd;
  int ret;
  fd_set rset, wset;
  FILE *inslip;
  const char *siodev = NULL;
  const char *dhcp_server = NULL;
  u_int16_t myport = BOOTPS, dhport = BOOTPS;
  int baudrate = -2;

  ip_id = getpid() * time(NULL);

  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  while((c = getopt(argc, argv, "B:D:hs:t:")) != -1) {
    switch (c) {
    case 'B':
      baudrate = atoi(optarg);
      break;

    case 'D':
      dhcp_server = optarg;
      break;

    case 's':
      if(strncmp("/dev/", optarg, 5) == 0) {
	siodev = optarg + 5;
      } else {
	siodev = optarg;
      }
      break;

    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
	strcpy(tundev, optarg + 5);
      } else {
	strcpy(tundev, optarg);
      }
      break;

    case '?':
    case 'h':
    default:
      err(1, "usage: tunslip [-B baudrate] [-s siodev] [-t tundev] [-D dhcp-server] ipaddress netmask [dhcp-server]");
      break;
    }
  }
  argc -= (optind - 1);
  argv += (optind - 1);

  if(argc != 3 && argc != 4) {
    err(1, "usage: tunslip [-s siodev] [-t tundev] [-D dhcp-server] ipaddress netmask [dhcp-server]");
  }
  ipaddr = argv[1];
  netmask = argv[2];
  circuit_addr = inet_addr(ipaddr);
  netaddr = inet_addr(ipaddr) & inet_addr(netmask);

  switch(baudrate) {
  case -2:
    break;			/* Use default. */
  case 9600:
    b_rate = B9600;
    break;
  case 19200:
    b_rate = B19200;
    break;
  case 38400:
    b_rate = B38400;
    break;
  case 57600:
    b_rate = B57600;
    break;
  case 115200:
    b_rate = B115200;
    break;
  default:
    err(1, "unknown baudrate %d", baudrate);
    break;
  }

  /*
   * Set up DHCP relay agent socket and find the address of this relay
   * agent.
   */
  if(argc == 4) {
    dhcp_server = argv[3];
  }
  if(dhcp_server != NULL) {
    struct sockaddr_in myaddr;
    socklen_t len;
    in_addr_t a;

    if(strchr(dhcp_server, ':') != NULL) {
      dhport = atoi(strchr(dhcp_server, ':') + 1);
      myport = dhport + 1;
      *strchr(dhcp_server, ':') = '\0';
    }
    a = inet_addr(dhcp_server);
    if(a == -1) {
      err(1, "illegal dhcp-server address");
    }
#ifndef linux
    dhaddr.sin_len = sizeof(dhaddr);
#endif
    dhaddr.sin_family = AF_INET;
    dhaddr.sin_port = htons(dhport);
    dhaddr.sin_addr.s_addr = a;

    dhsock = socket(AF_INET, SOCK_DGRAM, 0);
    if(dhsock < 0) {
      err (1, "socket");
    }
    memset(&myaddr, 0x0, sizeof(myaddr));
#ifndef linux
    myaddr.sin_len = sizeof(myaddr);
#endif
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(myport);
    if(bind(dhsock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
      err(1, "bind dhcp-relay");
    }

    if(connect(dhsock, (struct sockaddr *)&dhaddr, sizeof(dhaddr)) < 0) {
      err(1, "connect to dhcp-server");
    }

    len = sizeof(myaddr);
    if(getsockname(dhsock, (struct sockaddr *)&myaddr, &len) < 0) {
      err(1, "getsockname dhsock");
    }

    giaddr = myaddr.sin_addr.s_addr;

    /*
     * Don't want connected socket.
     */
    close(dhsock);
    dhsock = socket(AF_INET, SOCK_DGRAM, 0);
    if(dhsock < 0) {
      err (1, "socket");
    }
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(myport);
    if(bind(dhsock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
      err(1, "bind dhcp-relay");
    }
    fprintf(stderr, "DHCP server at %s:%d\n", dhcp_server, dhport);
  }

  if(siodev != NULL) {
      slipfd = devopen(siodev, O_RDWR | O_NONBLOCK);
      if(slipfd == -1) {
	err(1, "can't open siodev ``/dev/%s''", siodev);
      }
  } else {
    static const char *siodevs[] = {
      "ttyUSB0", "cuaU0", "ucom0" /* linux, fbsd6, fbsd5 */
    };
    int i;
    for(i = 0; i < 3; i++) {
      siodev = siodevs[i];
      slipfd = devopen(siodev, O_RDWR | O_NONBLOCK);
      if (slipfd != -1)
	break;
    }
    if(slipfd == -1) {
      err(1, "can't open siodev");
    }
  }
  fprintf(stderr, "slip started on ``/dev/%s''\n", siodev);
  stty_telos(slipfd);
  slip_send(slipfd, SLIP_END);
  inslip = fdopen(slipfd, "r");
  if(inslip == NULL) err(1, "main: fdopen");

  tunfd = tun_alloc(tundev);
  if(tunfd == -1) err(1, "main: open");
  fprintf(stderr, "opened device ``/dev/%s''\n", tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  signal(SIGALRM, sigalarm);
  ifconf(tundev, ipaddr, netmask);

  while(1) {
    maxfd = 0;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    if(got_sigalarm) {
      /* Send "?IPA". */
      slip_send(slipfd, '?');
      slip_send(slipfd, 'I');
      slip_send(slipfd, 'P');
      slip_send(slipfd, 'A');
      slip_send(slipfd, SLIP_END);
      got_sigalarm = 0;
    }

    if(!slip_empty()) {		/* Anything to flush? */
      FD_SET(slipfd, &wset);
    }

    FD_SET(slipfd, &rset);	/* Read from slip ASAP! */
    if(slipfd > maxfd) maxfd = slipfd;

    /* We only have one packet at a time queued for slip output. */
    if(slip_empty()) {
      FD_SET(tunfd, &rset);
      if(tunfd > maxfd) maxfd = tunfd;
      if(dhsock != -1) {
	FD_SET(dhsock, &rset);
	if(dhsock > maxfd) maxfd = dhsock;
      }
    }

    ret = select(maxfd + 1, &rset, &wset, NULL, NULL);
    if(ret == -1 && errno != EINTR) {
      err(1, "select");
    } else if(ret > 0) {
      if(FD_ISSET(slipfd, &rset)) {
        serial_to_tun(inslip, tunfd);
      }

      if(FD_ISSET(slipfd, &wset)) {
	slip_flushbuf(slipfd);
	sigalarm_reset();
      }

      if(slip_empty() && FD_ISSET(tunfd, &rset)) {
        tun_to_serial(tunfd, slipfd);
	slip_flushbuf(slipfd);
	sigalarm_reset();
      }

      if(dhsock != -1 && slip_empty() && FD_ISSET(dhsock, &rset)) {
	relay_dhcp_to_client(slipfd);
	slip_flushbuf(slipfd);
      }
    }
  }
}
