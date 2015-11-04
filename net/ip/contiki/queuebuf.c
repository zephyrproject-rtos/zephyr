/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Implementation of the Rime queue buffers
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup rimequeuebuf
 * @{
 */

#include "contiki-net.h"
#include "memb.h"
#include "queuebuf.h"
#if WITH_SWAP
#include "cfs/cfs.h"
#endif

#include <string.h> /* for memcpy() */

#ifdef QUEUEBUF_CONF_REF_NUM
#define QUEUEBUF_REF_NUM QUEUEBUF_CONF_REF_NUM
#else
#define QUEUEBUF_REF_NUM 2
#endif

/* Structure pointing to a buffer either stored
   in RAM or swapped in CFS */
struct queuebuf {
#if QUEUEBUF_DEBUG
  struct queuebuf *next;
  const char *file;
  int line;
  clock_time_t time;
#endif /* QUEUEBUF_DEBUG */
#if WITH_SWAP
  enum {IN_RAM, IN_CFS} location;
  union {
#endif
    struct queuebuf_data *ram_ptr;
#if WITH_SWAP
    int swap_id;
  };
#endif
};

/* The actual queuebuf data */
struct queuebuf_data {
  uint8_t data[PACKETBUF_SIZE];
  uint16_t len;
  struct packetbuf_attr attrs[PACKETBUF_NUM_ATTRS];
  struct packetbuf_addr addrs[PACKETBUF_NUM_ADDRS];
};

struct queuebuf_ref {
  uint16_t len;
  uint8_t *ref;
  uint8_t hdr[PACKETBUF_HDR_SIZE];
  uint8_t hdrlen;
};

MEMB(bufmem, struct queuebuf, QUEUEBUF_NUM);
MEMB(refbufmem, struct queuebuf_ref, QUEUEBUF_REF_NUM);
MEMB(buframmem, struct queuebuf_data, QUEUEBUFRAM_NUM);

#if WITH_SWAP

/* Swapping allows to store up to QUEUEBUF_NUM - QUEUEBUFRAM_NUM
   queuebufs in CFS. The swap is made of several large CFS files.
   Every buffer stored in CFS has a swap id, referring to a specific
   offset in one of these files. */
#define NQBUF_FILES 4
#define NQBUF_PER_FILE 256
#define QBUF_FILE_SIZE (NQBUF_PER_FILE*sizeof(struct queuebuf_data))
#define NQBUF_ID (NQBUF_PER_FILE * NQBUF_FILES)

struct qbuf_file {
  int fd;
  int usage;
  int renewable;
};

/* A statically allocated queuebuf used as a cache for swapped qbufs */
static struct queuebuf_data tmpdata;
/* A pointer to the qbuf associated to the data in tmpdata */
static struct queuebuf *tmpdata_qbuf = NULL;
/* The swap id counter */
static int next_swap_id = 0;
/* The swap files */
static struct qbuf_file qbuf_files[NQBUF_FILES];
/* The timer used to renew files during inactivity periods */
static struct ctimer renew_timer;

#endif

#if QUEUEBUF_DEBUG
#include "lib/list.h"
LIST(queuebuf_list);
#endif /* QUEUEBUF_DEBUG */

#define DEBUG 0
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#ifdef QUEUEBUF_CONF_STATS
#define QUEUEBUF_STATS QUEUEBUF_CONF_STATS
#else
#define QUEUEBUF_STATS 0
#endif /* QUEUEBUF_CONF_STATS */

#if QUEUEBUF_STATS
uint8_t queuebuf_len, queuebuf_ref_len, queuebuf_max_len;
#endif /* QUEUEBUF_STATS */

#if WITH_SWAP
/*---------------------------------------------------------------------------*/
static void
qbuf_renew_file(int file)
{
  int ret;
  char name[2];
  name[0] = 'a' + file;
  name[1] = '\0';
  if(qbuf_files[file].renewable == 1) {
    PRINTF("qbuf_renew_file: removing file %d\n", file);
    cfs_remove(name);
  }
  ret = cfs_open(name, CFS_READ | CFS_WRITE);
  if(ret == -1) {
    PRINTF("qbuf_renew_file: cfs open error\n");
  }
  qbuf_files[file].fd = ret;
  qbuf_files[file].usage = 0;
  qbuf_files[file].renewable = 0;
}
/*---------------------------------------------------------------------------*/
/* Renews every file with renewable flag set */
static void
qbuf_renew_all(void *unused)
{
  int i;
  for(i=0; i<NQBUF_FILES; i++) {
    if(qbuf_files[i].renewable == 1) {
      qbuf_renew_file(i);
    }
  }
}
/*---------------------------------------------------------------------------*/
/* Removes a queuebuf from its swap file */
static void
queuebuf_remove_from_file(int swap_id)
{
  int fileid;
  if(swap_id != -1) {
    fileid = swap_id / NQBUF_PER_FILE;
    qbuf_files[fileid].usage--;

    /* The file is full but doesn't contain any more queuebuf, mark it as renewable */
    if(qbuf_files[fileid].usage == 0 && fileid != next_swap_id / NQBUF_PER_FILE) {
      qbuf_files[fileid].renewable = 1;
      /* This file is renewable, set a timer to renew files */
      ctimer_set(&renew_timer, 0, qbuf_renew_all, NULL);
    }

    if(tmpdata_qbuf->swap_id == swap_id) {
      tmpdata_qbuf->swap_id = -1;
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
get_new_swap_id(void)
{
  int fileid;
  int swap_id = next_swap_id;
  fileid = swap_id / NQBUF_PER_FILE;
  if(swap_id % NQBUF_PER_FILE == 0) { /* This is the first id in the file */
    if(qbuf_files[fileid].renewable) {
      qbuf_renew_file(fileid);
    }
    if(qbuf_files[fileid].usage>0) {
      return -1;
    }
  }
  qbuf_files[fileid].usage++;
  next_swap_id = (next_swap_id+1) % NQBUF_ID;
  return swap_id;
}
/*---------------------------------------------------------------------------*/
/* Flush tmpdata to CFS */
static int
queuebuf_flush_tmpdata(void)
{
  int fileid, fd, ret;
  cfs_offset_t offset;
  if(tmpdata_qbuf) {
    queuebuf_remove_from_file(tmpdata_qbuf->swap_id);
    tmpdata_qbuf->swap_id = get_new_swap_id();
    if(tmpdata_qbuf->swap_id == -1) {
      return -1;
    }
    fileid = tmpdata_qbuf->swap_id / NQBUF_PER_FILE;
    offset = (tmpdata_qbuf->swap_id % NQBUF_PER_FILE) * sizeof(struct queuebuf_data);
    fd = qbuf_files[fileid].fd;
    ret = cfs_seek(fd, offset, CFS_SEEK_SET);
    if(ret == -1) {
      PRINTF("queuebuf_flush_tmpdata: cfs seek error\n");
      return -1;
    }
    ret = cfs_write(fd, &tmpdata, sizeof(struct queuebuf_data));
    if(ret == -1) {
      PRINTF("queuebuf_flush_tmpdata: cfs write error\n");
      return -1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* If the queuebuf is in CFS, load it to tmpdata */
static struct queuebuf_data *
queuebuf_load_to_ram(struct queuebuf *b)
{
  int fileid, fd, ret;
  cfs_offset_t offset;
  if(b->location == IN_RAM) { /* the qbuf is loacted in RAM */
    return b->ram_ptr;
  } else { /* the qbuf is located in CFS */
    if(tmpdata_qbuf && tmpdata_qbuf->swap_id == b->swap_id) { /* the qbuf is already in tmpdata */
      return &tmpdata;
    } else { /* the qbuf needs to be loaded from CFS */
      tmpdata_qbuf = b;
      /* read the qbuf from CFS */
      fileid = b->swap_id / NQBUF_PER_FILE;
      offset = (b->swap_id % NQBUF_PER_FILE) * sizeof(struct queuebuf_data);
      fd = qbuf_files[fileid].fd;
      ret = cfs_seek(fd, offset, CFS_SEEK_SET);
      if(ret == -1) {
        PRINTF("queuebuf_load_to_ram: cfs seek error\n");
      }
      ret = cfs_read(fd, &tmpdata, sizeof(struct queuebuf_data));
      if(ret == -1) {
        PRINTF("queuebuf_load_to_ram: cfs read error\n");
      }
      return &tmpdata;
    }
  }
}
#else /* WITH_SWAP */
/*---------------------------------------------------------------------------*/
static struct queuebuf_data *
queuebuf_load_to_ram(struct queuebuf *b)
{
  return b->ram_ptr;
}
#endif /* WITH_SWAP */
/*---------------------------------------------------------------------------*/
void
queuebuf_init(void)
{
#if WITH_SWAP
  int i;
  for(i=0; i<NQBUF_FILES; i++) {
    qbuf_files[i].renewable = 1;
    qbuf_renew_file(i);
  }
#endif
  memb_init(&buframmem);
  memb_init(&bufmem);
  memb_init(&refbufmem);
#if QUEUEBUF_STATS
  queuebuf_max_len = QUEUEBUF_NUM;
#endif /* QUEUEBUF_STATS */
}
/*---------------------------------------------------------------------------*/
int
queuebuf_numfree(struct net_buf *buf)
{
  if(packetbuf_is_reference(buf)) {
    return memb_numfree(&refbufmem);
  } else {
    return memb_numfree(&bufmem);
  }
}
/*---------------------------------------------------------------------------*/
#if QUEUEBUF_DEBUG
struct queuebuf *
queuebuf_new_from_packetbuf_debug(struct net_buf *netbuf, const char *file, int line)
#else /* QUEUEBUF_DEBUG */
struct queuebuf *
queuebuf_new_from_packetbuf(struct net_buf *netbuf)
#endif /* QUEUEBUF_DEBUG */
{
  struct queuebuf *buf;
  struct queuebuf_ref *rbuf;

  if(packetbuf_is_reference(netbuf)) {
    rbuf = memb_alloc(&refbufmem);
    if(rbuf != NULL) {
#if QUEUEBUF_STATS
      ++queuebuf_ref_len;
#endif /* QUEUEBUF_STATS */
      rbuf->len = packetbuf_datalen(netbuf);
      rbuf->ref = packetbuf_reference_ptr(netbuf);
      rbuf->hdrlen = packetbuf_copyto_hdr(netbuf, rbuf->hdr);
    } else {
      PRINTF("queuebuf_new_from_packetbuf: could not allocate a reference queuebuf\n");
    }
    return (struct queuebuf *)rbuf;
  } else {
    struct queuebuf_data *buframptr;
    buf = memb_alloc(&bufmem);
    if(buf != NULL) {
#if QUEUEBUF_DEBUG
      list_add(queuebuf_list, buf);
      buf->file = file;
      buf->line = line;
      buf->time = clock_time();
#endif /* QUEUEBUF_DEBUG */
      buf->ram_ptr = memb_alloc(&buframmem);
#if WITH_SWAP
      /* If the allocation failed, store the qbuf in swap files */
      if(buf->ram_ptr != NULL) {
        buf->location = IN_RAM;
        buframptr = buf->ram_ptr;
      } else {
        buf->location = IN_CFS;
        buf->swap_id = -1;
        tmpdata_qbuf = buf;
        buframptr = &tmpdata;
      }
#else
      if(buf->ram_ptr == NULL) {
        PRINTF("queuebuf_new_from_packetbuf: could not queuebuf data\n");
        memb_free(&bufmem, buf);
        return NULL;
      }
      buframptr = buf->ram_ptr;
#endif

      buframptr->len = packetbuf_copyto(netbuf, buframptr->data);
      packetbuf_attr_copyto(netbuf, buframptr->attrs, buframptr->addrs);

#if WITH_SWAP
      if(buf->location == IN_CFS) {
        if(queuebuf_flush_tmpdata() == -1) {
          /* We were unable to write the data in the swap */
          memb_free(&bufmem, buf);
          return NULL;
        }
      }
#endif

#if QUEUEBUF_STATS
      ++queuebuf_len;
      PRINTF("queuebuf len %d\n", queuebuf_len);
      printf("#A q=%d\n", queuebuf_len);
      if(queuebuf_len == queuebuf_max_len + 1) {
        queuebuf_free(buf);
        queuebuf_len--;
        return NULL;
      }
#endif /* QUEUEBUF_STATS */

    } else {
      PRINTF("queuebuf_new_from_packetbuf: could not allocate a queuebuf\n");
    }
    return buf;
  }
}
/*---------------------------------------------------------------------------*/
void
queuebuf_update_attr_from_packetbuf(struct net_buf *netbuf, struct queuebuf *buf)
{
  struct queuebuf_data *buframptr = queuebuf_load_to_ram(buf);
  packetbuf_attr_copyto(netbuf, buframptr->attrs, buframptr->addrs);
#if WITH_SWAP
  if(buf->location == IN_CFS) {
    queuebuf_flush_tmpdata();
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
queuebuf_update_from_packetbuf(struct net_buf *netbuf, struct queuebuf *buf)
{
  struct queuebuf_data *buframptr = queuebuf_load_to_ram(buf);
  packetbuf_attr_copyto(netbuf, buframptr->attrs, buframptr->addrs);
  buframptr->len = packetbuf_copyto(netbuf, buframptr->data);
#if WITH_SWAP
  if(buf->location == IN_CFS) {
    queuebuf_flush_tmpdata();
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
queuebuf_free(struct queuebuf *buf)
{
  if(memb_inmemb(&bufmem, buf)) {
#if WITH_SWAP
    if(buf->location == IN_RAM) {
      memb_free(&buframmem, buf->ram_ptr);
    } else {
      queuebuf_remove_from_file(buf->swap_id);
    }
#else
    memb_free(&buframmem, buf->ram_ptr);
#endif
    memb_free(&bufmem, buf);
#if QUEUEBUF_STATS
    --queuebuf_len;
    printf("#A q=%d\n", queuebuf_len);
#endif /* QUEUEBUF_STATS */
#if QUEUEBUF_DEBUG
    list_remove(queuebuf_list, buf);
#endif /* QUEUEBUF_DEBUG */
  } else if(memb_inmemb(&refbufmem, buf)) {
    memb_free(&refbufmem, buf);
#if QUEUEBUF_STATS
    --queuebuf_ref_len;
#endif /* QUEUEBUF_STATS */
  }
}
/*---------------------------------------------------------------------------*/
void
queuebuf_to_packetbuf(struct net_buf *netbuf, struct queuebuf *b)
{
  struct queuebuf_ref *r;
  if(memb_inmemb(&bufmem, b)) {
    struct queuebuf_data *buframptr = queuebuf_load_to_ram(b);
    packetbuf_copyfrom(netbuf, buframptr->data, buframptr->len);
    packetbuf_attr_copyfrom(netbuf, buframptr->attrs, buframptr->addrs);
  } else if(memb_inmemb(&refbufmem, b)) {
    r = (struct queuebuf_ref *)b;
    packetbuf_clear(netbuf);
    packetbuf_copyfrom(netbuf, r->ref, r->len);
    packetbuf_hdralloc(netbuf, r->hdrlen);
    memcpy(packetbuf_hdrptr(netbuf), r->hdr, r->hdrlen);
  }
}
/*---------------------------------------------------------------------------*/
void *
queuebuf_dataptr(struct queuebuf *b)
{
  struct queuebuf_ref *r;

  if(memb_inmemb(&bufmem, b)) {
    struct queuebuf_data *buframptr = queuebuf_load_to_ram(b);
    return buframptr->data;
  } else if(memb_inmemb(&refbufmem, b)) {
    r = (struct queuebuf_ref *)b;
    return r->ref;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
queuebuf_datalen(struct queuebuf *b)
{
  struct queuebuf_data *buframptr = queuebuf_load_to_ram(b);
  return buframptr->len;
}
/*---------------------------------------------------------------------------*/
linkaddr_t *
queuebuf_addr(struct queuebuf *b, uint8_t type)
{
  struct queuebuf_data *buframptr = queuebuf_load_to_ram(b);
  return &buframptr->addrs[type - PACKETBUF_ADDR_FIRST].addr;
}
/*---------------------------------------------------------------------------*/
packetbuf_attr_t
queuebuf_attr(struct queuebuf *b, uint8_t type)
{
  struct queuebuf_data *buframptr = queuebuf_load_to_ram(b);
  return buframptr->attrs[type].val;
}
/*---------------------------------------------------------------------------*/
void
queuebuf_debug_print(void)
{
#if QUEUEBUF_DEBUG
  struct queuebuf *q;
  printf("queuebuf_list: ");
  for(q = list_head(queuebuf_list); q != NULL;
      q = list_item_next(q)) {
    printf("%s,%d,%lu ", q->file, q->line, q->time);
  }
  printf("\n");
#endif /* QUEUEBUF_DEBUG */
}
/*---------------------------------------------------------------------------*/
/** @} */
