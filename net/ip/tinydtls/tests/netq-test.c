#include <string.h>
#include <netinet/in.h>

#include "netq.h" 

#ifndef NDEBUG
extern void nq_dump(struct netq_t *);
#endif

int main(int argc, char **argv) {
#ifndef NDEBUG
  struct netq_t *nq;

  struct sockaddr_in6 dst = { AF_INET6, htons(20220), 0, IN6ADDR_ANY_INIT, 0 };
  struct packet_t *p;

  char *pkt[20] = { 
    "Packet #1",
    "This is packet #2",
    "The third packet #3 is the largest",
    "Packet #4",
    "Packet #5",
    "Packet #6",
    "Packet #7"
  };

  nq = nq_new(200);

  if (!nq) {
    fprintf(stderr, "E: cannot create network packet queue\n");
    return -1;
  }

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[0], strlen(pkt[0]))) {
    fprintf(stderr, "E: cannot add packet #1\n");
  }

  nq_dump(nq);

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[1], strlen(pkt[1]))) {
    fprintf(stderr, "E: cannot add packet #2\n");
  }

  nq_dump(nq);

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[2], strlen(pkt[2]))) {
    fprintf(stderr, "E: cannot add packet #3\n");
  }

  nq_dump(nq);

  p = nq_pop(nq);
  if (!p) {
    fprintf(stderr, "E: no packet\n");
  }

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[3], strlen(pkt[3]))) {
    fprintf(stderr, "E: cannot add packet #4\n");
  }

  nq_dump(nq);

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[4], strlen(pkt[4]))) {
    fprintf(stderr, "E: cannot add packet #5\n");
  }

  nq_dump(nq);

  p = nq_pop(nq);
  if (!p) {
    fprintf(stderr, "E: no packet\n");
  }

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[5], strlen(pkt[5]))) {
    fprintf(stderr, "E: cannot add packet #6\n");
  }

  nq_dump(nq);

  p = nq_pop(nq);
  p = nq_pop(nq);
  p = nq_pop(nq);
  p = nq_pop(nq);
  p = nq_pop(nq);
  p = nq_pop(nq);
  p = nq_pop(nq);

  if (!nq_new_packet(nq, (struct sockaddr *)&dst, sizeof(dst), 
		     0, pkt[6], strlen(pkt[6]))) {
    fprintf(stderr, "E: cannot add packet #7\n");
  }

  nq_dump(nq);
#endif

  return 0;
}
