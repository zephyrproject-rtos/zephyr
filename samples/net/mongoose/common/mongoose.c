// Copyright (c) 2004-2013 Sergey Lyubka
// Copyright (c) 2013-2022 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see http://www.gnu.org/licenses/
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in https://www.mongoose.ws/licensing/
//
// SPDX-License-Identifier: GPL-2.0 or commercial

#include "mongoose.h"

#ifdef MG_ENABLE_LINES
#line 1 "src/base64.c"
#endif

#include <string.h>

static int mg_b64idx(int c) {
  if (c < 26) {
    return c + 'A';
  } else if (c < 52) {
    return c - 26 + 'a';
  } else if (c < 62) {
    return c - 52 + '0';
  } else {
    return c == 62 ? '+' : '/';
  }
}

static int mg_b64rev(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  } else if (c >= 'a' && c <= 'z') {
    return c + 26 - 'a';
  } else if (c >= '0' && c <= '9') {
    return c + 52 - '0';
  } else if (c == '+') {
    return 62;
  } else if (c == '/') {
    return 63;
  } else if (c == '=') {
    return 64;
  } else {
    return -1;
  }
}

int mg_base64_update(unsigned char ch, char *to, int n) {
  int rem = (n & 3) % 3;
  if (rem == 0) {
    to[n] = (char) mg_b64idx(ch >> 2);
    to[++n] = (char) ((ch & 3) << 4);
  } else if (rem == 1) {
    to[n] = (char) mg_b64idx(to[n] | (ch >> 4));
    to[++n] = (char) ((ch & 15) << 2);
  } else {
    to[n] = (char) mg_b64idx(to[n] | (ch >> 6));
    to[++n] = (char) mg_b64idx(ch & 63);
    n++;
  }
  return n;
}

int mg_base64_final(char *to, int n) {
  int saved = n;
  // printf("---[%.*s]\n", n, to);
  if (n & 3) n = mg_base64_update(0, to, n);
  if ((saved & 3) == 2) n--;
  // printf("    %d[%.*s]\n", n, n, to);
  while (n & 3) to[n++] = '=';
  to[n] = '\0';
  return n;
}

int mg_base64_encode(const unsigned char *p, int n, char *to) {
  int i, len = 0;
  for (i = 0; i < n; i++) len = mg_base64_update(p[i], to, len);
  len = mg_base64_final(to, len);
  return len;
}

int mg_base64_decode(const char *src, int n, char *dst) {
  const char *end = src + n;
  int len = 0;
  while (src + 3 < end) {
    int a = mg_b64rev(src[0]), b = mg_b64rev(src[1]), c = mg_b64rev(src[2]),
        d = mg_b64rev(src[3]);
    if (a == 64 || a < 0 || b == 64 || b < 0 || c < 0 || d < 0) return 0;
    dst[len++] = (char) ((a << 2) | (b >> 4));
    if (src[2] != '=') {
      dst[len++] = (char) ((b << 4) | (c >> 2));
      if (src[3] != '=') dst[len++] = (char) ((c << 6) | d);
    }
    src += 4;
  }
  dst[len] = '\0';
  return len;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/dns.c"
#endif







struct dns_data {
  struct dns_data *next;
  struct mg_connection *c;
  int64_t expire;
  uint16_t txnid;
};

static struct dns_data *s_reqs;  // Active DNS requests

static void mg_sendnsreq(struct mg_connection *, struct mg_str *, int,
                         struct mg_dns *, bool);

static void mg_dns_free(struct dns_data *d) {
  LIST_DELETE(struct dns_data, &s_reqs, d);
  free(d);
}

void mg_resolve_cancel(struct mg_connection *c) {
  struct dns_data *tmp, *d;
  for (d = s_reqs; d != NULL; d = tmp) {
    tmp = d->next;
    if (d->c == c) mg_dns_free(d);
  }
}

static size_t mg_dns_parse_name_depth(const uint8_t *s, size_t len, size_t ofs,
                                      char *to, size_t tolen, size_t j,
                                      int depth) {
  size_t i = 0;
  if (tolen > 0 && depth == 0) to[0] = '\0';
  if (depth > 5) return 0;
  // MG_INFO(("ofs %lx %x %x", (unsigned long) ofs, s[ofs], s[ofs + 1]));
  while (ofs + i + 1 < len) {
    size_t n = s[ofs + i];
    if (n == 0) {
      i++;
      break;
    }
    if (n & 0xc0) {
      size_t ptr = (((n & 0x3f) << 8) | s[ofs + i + 1]);  // 12 is hdr len
      // MG_INFO(("PTR %lx", (unsigned long) ptr));
      if (ptr + 1 < len && (s[ptr] & 0xc0) == 0 &&
          mg_dns_parse_name_depth(s, len, ptr, to, tolen, j, depth + 1) == 0)
        return 0;
      i += 2;
      break;
    }
    if (ofs + i + n + 1 >= len) return 0;
    if (j > 0) {
      if (j < tolen) to[j] = '.';
      j++;
    }
    if (j + n < tolen) memcpy(&to[j], &s[ofs + i + 1], n);
    j += n;
    i += n + 1;
    if (j < tolen) to[j] = '\0';  // Zero-terminate this chunk
    // MG_INFO(("--> [%s]", to));
  }
  if (tolen > 0) to[tolen - 1] = '\0';  // Make sure make sure it is nul-term
  return i;
}

static size_t mg_dns_parse_name(const uint8_t *s, size_t n, size_t ofs,
                                char *dst, size_t dstlen) {
  return mg_dns_parse_name_depth(s, n, ofs, dst, dstlen, 0, 0);
}

size_t mg_dns_parse_rr(const uint8_t *buf, size_t len, size_t ofs,
                       bool is_question, struct mg_dns_rr *rr) {
  const uint8_t *s = buf + ofs, *e = &buf[len];

  memset(rr, 0, sizeof(*rr));
  if (len < sizeof(struct mg_dns_header)) return 0;  // Too small
  if (len > 512) return 0;  //  Too large, we don't expect that
  if (s >= e) return 0;     //  Overflow

  if ((rr->nlen = (uint16_t) mg_dns_parse_name(buf, len, ofs, NULL, 0)) == 0)
    return 0;
  s += rr->nlen + 4;
  if (s > e) return 0;
  rr->atype = (uint16_t) (((uint16_t) s[-4] << 8) | s[-3]);
  rr->aclass = (uint16_t) (((uint16_t) s[-2] << 8) | s[-1]);
  if (is_question) return (size_t) (rr->nlen + 4);

  s += 6;
  if (s > e) return 0;
  rr->alen = (uint16_t) (((uint16_t) s[-2] << 8) | s[-1]);
  if (s + rr->alen > e) return 0;
  return (size_t) (rr->nlen + rr->alen + 10);
}

bool mg_dns_parse(const uint8_t *buf, size_t len, struct mg_dns_message *dm) {
  const struct mg_dns_header *h = (struct mg_dns_header *) buf;
  struct mg_dns_rr rr;
  size_t i, n, ofs = sizeof(*h);
  memset(dm, 0, sizeof(*dm));

  if (len < sizeof(*h)) return 0;                // Too small, headers dont fit
  if (mg_ntohs(h->num_questions) > 1) return 0;  // Sanity
  if (mg_ntohs(h->num_answers) > 10) return 0;   // Sanity
  dm->txnid = mg_ntohs(h->txnid);

  for (i = 0; i < mg_ntohs(h->num_questions); i++) {
    if ((n = mg_dns_parse_rr(buf, len, ofs, true, &rr)) == 0) return false;
    // MG_INFO(("Q %lu %lu %hu/%hu", ofs, n, rr.atype, rr.aclass));
    ofs += n;
  }
  for (i = 0; i < mg_ntohs(h->num_answers); i++) {
    if ((n = mg_dns_parse_rr(buf, len, ofs, false, &rr)) == 0) return false;
    // MG_INFO(("A -- %lu %lu %hu/%hu %s", ofs, n, rr.atype, rr.aclass,
    // dm->name));
    mg_dns_parse_name(buf, len, ofs, dm->name, sizeof(dm->name));
    ofs += n;

    if (rr.alen == 4 && rr.atype == 1 && rr.aclass == 1) {
      dm->addr.is_ip6 = false;
      memcpy(&dm->addr.ip, &buf[ofs - 4], 4);
      dm->resolved = true;
      break;  // Return success
    } else if (rr.alen == 16 && rr.atype == 28 && rr.aclass == 1) {
      dm->addr.is_ip6 = true;
      memcpy(&dm->addr.ip6, &buf[ofs - 16], 16);
      dm->resolved = true;
      break;  // Return success
    }
  }
  return true;
}

static void dns_cb(struct mg_connection *c, int ev, void *ev_data,
                   void *fn_data) {
  struct dns_data *d, *tmp;
  if (ev == MG_EV_POLL) {
    int64_t now = *(int64_t *) ev_data;
    for (d = s_reqs; d != NULL; d = tmp) {
      tmp = d->next;
      // MG_DEBUG ("%lu %lu dns poll", d->expire, now));
      if (now > d->expire) mg_error(d->c, "DNS timeout");
    }
  } else if (ev == MG_EV_READ) {
    struct mg_dns_message dm;
    int resolved = 0;
    if (mg_dns_parse(c->recv.buf, c->recv.len, &dm) == false) {
      char *s = mg_hexdump(c->recv.buf, c->recv.len);
      MG_ERROR(("Unexpected DNS response:\n%s\n", s));
      free(s);
    } else {
      MG_VERBOSE(("%s %d", dm.name, dm.resolved));
      for (d = s_reqs; d != NULL; d = tmp) {
        tmp = d->next;
        // MG_INFO(("d %p %hu %hu", d, d->txnid, dm.txnid));
        if (dm.txnid != d->txnid) continue;
        if (d->c->is_resolving) {
          if (dm.resolved) {
            char buf[100];
            dm.addr.port = d->c->rem.port;  // Save port
            d->c->rem = dm.addr;            // Copy resolved address
            MG_DEBUG(("%lu %s resolved to %s", d->c->id, dm.name,
                      mg_ntoa(&d->c->rem, buf, sizeof(buf))));
            mg_connect_resolved(d->c);
#if MG_ENABLE_IPV6
          } else if (dm.addr.is_ip6 == false && dm.name[0] != '\0') {
            struct mg_str x = mg_str(dm.name);
            mg_sendnsreq(d->c, &x, c->mgr->dnstimeout, &c->mgr->dns6, true);
#endif
          } else {
            mg_error(d->c, "%s DNS lookup failed", dm.name);
          }
        } else {
          MG_ERROR(("%lu already resolved", d->c->id));
        }
        mg_dns_free(d);
        resolved = 1;
      }
    }
    if (!resolved) MG_ERROR(("stray DNS reply"));
    c->recv.len = 0;
  } else if (ev == MG_EV_CLOSE) {
    for (d = s_reqs; d != NULL; d = tmp) {
      tmp = d->next;
      mg_error(d->c, "DNS error");
      mg_dns_free(d);
    }
  }
  (void) fn_data;
}

static void mg_dns_send(struct mg_connection *c, const struct mg_str *name,
                        uint16_t txnid, bool ipv6) {
  struct {
    struct mg_dns_header header;
    uint8_t data[256];
  } pkt;
  size_t i, n;
  memset(&pkt, 0, sizeof(pkt));
  pkt.header.txnid = mg_htons(txnid);
  pkt.header.flags = mg_htons(0x100);
  pkt.header.num_questions = mg_htons(1);
  for (i = n = 0; i < sizeof(pkt.data) - 5; i++) {
    if (name->ptr[i] == '.' || i >= name->len) {
      pkt.data[n] = (uint8_t) (i - n);
      memcpy(&pkt.data[n + 1], name->ptr + n, i - n);
      n = i + 1;
    }
    if (i >= name->len) break;
  }
  memcpy(&pkt.data[n], "\x00\x00\x01\x00\x01", 5);  // A query
  n += 5;
  if (ipv6) pkt.data[n - 3] = 0x1c;  // AAAA query
  // memcpy(&pkt.data[n], "\xc0\x0c\x00\x1c\x00\x01", 6);  // AAAA query
  // n += 6;
  mg_send(c, &pkt, sizeof(pkt.header) + n);
#if 0
  // Immediately after A query, send AAAA query. Whatever reply comes first,
  // we'll use it. Note: we cannot send two queries in a single packet.
  // https://stackoverflow.com/questions/4082081/requesting-a-and-aaaa-records-in-single-dns-query
  pkt.data[n - 3] = 0x1c;  // AAAA query
  mg_send(c, &pkt, sizeof(pkt.header) + n);
#endif
}

static void mg_sendnsreq(struct mg_connection *c, struct mg_str *name, int ms,
                         struct mg_dns *dnsc, bool ipv6) {
  struct dns_data *d = NULL;
  if (dnsc->url == NULL) {
    mg_error(c, "DNS server URL is NULL. Call mg_mgr_init()");
  } else if (dnsc->c == NULL) {
    dnsc->c = mg_connect(c->mgr, dnsc->url, NULL, NULL);
    if (dnsc->c != NULL) {
      dnsc->c->pfn = dns_cb;
      // dnsc->c->is_hexdumping = 1;
    }
  }
  if (dnsc->c == NULL) {
    mg_error(c, "resolver");
  } else if ((d = (struct dns_data *) calloc(1, sizeof(*d))) == NULL) {
    mg_error(c, "resolve OOM");
  } else {
    char buf[100];
    d->txnid = s_reqs ? (uint16_t) (s_reqs->txnid + 1) : 1;
    d->next = s_reqs;
    s_reqs = d;
    d->expire = mg_millis() + (int64_t) ms;
    d->c = c;
    c->is_resolving = 1;
    MG_VERBOSE(("%lu resolving %.*s @ %s, txnid %hu", c->id, (int) name->len,
                name->ptr, mg_ntoa(&dnsc->c->rem, buf, sizeof(buf)), d->txnid));
    mg_dns_send(dnsc->c, name, d->txnid, ipv6);
  }
}

void mg_resolve(struct mg_connection *c, const char *url) {
  struct mg_str host = mg_url_host(url);
  c->rem.port = mg_htons(mg_url_port(url));
  if (mg_aton(host, &c->rem)) {
    // host is an IP address, do not fire name resolution
    mg_connect_resolved(c);
  } else {
    // host is not an IP, send DNS resolution request
    mg_sendnsreq(c, &host, c->mgr->dnstimeout, &c->mgr->dns4, false);
  }
}

#ifdef MG_ENABLE_LINES
#line 1 "src/event.c"
#endif





void mg_call(struct mg_connection *c, int ev, void *ev_data) {
  // Run user-defined handler first, in order to give it an ability
  // to intercept processing (e.g. clean input buffer) before the
  // protocol handler kicks in
  if (c->fn != NULL) c->fn(c, ev, ev_data, c->fn_data);
  if (c->pfn != NULL) c->pfn(c, ev, ev_data, c->pfn_data);
}

void mg_error(struct mg_connection *c, const char *fmt, ...) {
  char mem[256], *buf = mem;
  va_list ap;
  va_start(ap, fmt);
  mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  va_end(ap);
  MG_ERROR(("%lu %s", c->id, buf));
  c->is_closing = 1;             // Set is_closing before sending MG_EV_CALL
  mg_call(c, MG_EV_ERROR, buf);  // Let user handler to override it
  if (buf != mem) free(buf);
}

#ifdef MG_ENABLE_LINES
#line 1 "src/fs.c"
#endif



struct mg_fd *mg_fs_open(struct mg_fs *fs, const char *path, int flags) {
  struct mg_fd *fd = (struct mg_fd *) calloc(1, sizeof(*fd));
  if (fd != NULL) {
    fd->fd = fs->op(path, flags);
    fd->fs = fs;
    if (fd->fd == NULL) {
      free(fd);
      fd = NULL;
    }
  }
  return fd;
}

void mg_fs_close(struct mg_fd *fd) {
  if (fd != NULL) {
    fd->fs->cl(fd->fd);
    free(fd);
  }
}

char *mg_file_read(struct mg_fs *fs, const char *path, size_t *sizep) {
  struct mg_fd *fd;
  char *data = NULL;
  size_t size = 0;
  fs->st(path, &size, NULL);
  if ((fd = mg_fs_open(fs, path, MG_FS_READ)) != NULL) {
    data = (char *) calloc(1, size + 1);
    if (data != NULL) {
      if (fs->rd(fd->fd, data, size) != size) {
        free(data);
        data = NULL;
      } else {
        data[size] = '\0';
        if (sizep != NULL) *sizep = size;
      }
    }
    mg_fs_close(fd);
  }
  return data;
}

bool mg_file_write(struct mg_fs *fs, const char *path, const void *buf,
                   size_t len) {
  bool result = false;
  struct mg_fd *fd;
  char tmp[MG_PATH_MAX];
  mg_snprintf(tmp, sizeof(tmp), "%s..%d", path, rand());
  if ((fd = mg_fs_open(fs, tmp, MG_FS_WRITE)) != NULL) {
    result = fs->wr(fd->fd, buf, len) == len;
    mg_fs_close(fd);
    if (result) {
      fs->rm(path);
      fs->mv(tmp, path);
    } else {
      fs->rm(tmp);
    }
  }
  return result;
}

bool mg_file_printf(struct mg_fs *fs, const char *path, const char *fmt, ...) {
  char tmp[256], *buf = tmp;
  bool result;
  size_t len;
  va_list ap;
  va_start(ap, fmt);
  len = mg_vasprintf(&buf, sizeof(tmp), fmt, ap);
  va_end(ap);
  result = mg_file_write(fs, path, buf, len > 0 ? (size_t) len : 0);
  if (buf != tmp) free(buf);
  return result;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/fs_fat.c"
#endif


#if MG_ENABLE_FATFS
#include <ff.h>

static int mg_days_from_epoch(int y, int m, int d) {
  y -= m <= 2;
  int era = y / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

static time_t mg_timegm(const struct tm *t) {
  int year = t->tm_year + 1900;
  int month = t->tm_mon;  // 0-11
  if (month > 11) {
    year += month / 12;
    month %= 12;
  } else if (month < 0) {
    int years_diff = (11 - month) / 12;
    year -= years_diff;
    month += 12 * years_diff;
  }
  int x = mg_days_from_epoch(year, month + 1, t->tm_mday);
  return 60 * (60 * (24L * x + t->tm_hour) + t->tm_min) + t->tm_sec;
}

static time_t ff_time_to_epoch(uint16_t fdate, uint16_t ftime) {
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  tm.tm_sec = (ftime << 1) & 0x3e;
  tm.tm_min = ((ftime >> 5) & 0x3f);
  tm.tm_hour = ((ftime >> 11) & 0x1f);
  tm.tm_mday = (fdate & 0x1f);
  tm.tm_mon = ((fdate >> 5) & 0x0f) - 1;
  tm.tm_year = ((fdate >> 9) & 0x7f) + 80;
  return mg_timegm(&tm);
}

static int ff_stat(const char *path, size_t *size, time_t *mtime) {
  FILINFO fi;
  if (path[0] == '\0' || strcmp(path, MG_FATFS_ROOT) == 0) {
    if (size) *size = 0;
    if (mtime) *mtime = 0;
    return MG_FS_DIR;
  } else if (f_stat(path, &fi) == 0) {
    if (size) *size = (size_t) fi.fsize;
    if (mtime) *mtime = ff_time_to_epoch(fi.fdate, fi.ftime);
    return MG_FS_READ | MG_FS_WRITE | ((fi.fattrib & AM_DIR) ? MG_FS_DIR : 0);
  } else {
    return 0;
  }
}

static void ff_list(const char *dir, void (*fn)(const char *, void *),
                    void *userdata) {
  DIR d;
  FILINFO fi;
  if (f_opendir(&d, dir) == FR_OK) {
    while (f_readdir(&d, &fi) == FR_OK && fi.fname[0] != '\0') {
      if (!strcmp(fi.fname, ".") || !strcmp(fi.fname, "..")) continue;
      fn(fi.fname, userdata);
    }
    f_closedir(&d);
  }
}

static void *ff_open(const char *path, int flags) {
  FIL f;
  unsigned char mode = FA_READ;
  if (flags & MG_FS_WRITE) mode |= FA_WRITE | FA_OPEN_ALWAYS | FA_OPEN_APPEND;
  if (f_open(&f, path, mode) == 0) {
    FIL *fp = calloc(1, sizeof(*fp));
    *fp = f;
    return fp;
  } else {
    return NULL;
  }
}

static void ff_close(void *fp) {
  if (fp != NULL) {
    f_close((FIL *) fp);
    free(fp);
  }
}

static size_t ff_read(void *fp, void *buf, size_t len) {
  unsigned n = 0, misalign = ((size_t) buf) & 3;
  if (misalign) {
    char aligned[4];
    f_read((FIL *) fp, aligned, len > misalign ? misalign : len, &n);
    memcpy(buf, aligned, n);
  } else {
    f_read((FIL *) fp, buf, len, &n);
  }
  return n;
}

static size_t ff_write(void *fp, const void *buf, size_t len) {
  unsigned n = 0;
  return f_write((FIL *) fp, (char *) buf, len, &n) == FR_OK ? n : 0;
}

static size_t ff_seek(void *fp, size_t offset) {
  f_lseek((FIL *) fp, offset);
  return offset;
}

static bool ff_rename(const char *from, const char *to) {
  return f_rename(from, to) == FR_OK;
}

static bool ff_remove(const char *path) {
  return f_unlink(path) == FR_OK;
}

static bool ff_mkdir(const char *path) {
  return f_mkdir(path) == FR_OK;
}

struct mg_fs mg_fs_fat = {ff_stat,  ff_list, ff_open,   ff_close,  ff_read,
                          ff_write, ff_seek, ff_rename, ff_remove, ff_mkdir};
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/fs_packed.c"
#endif



struct packed_file {
  const char *data;
  size_t size;
  size_t pos;
};

const char *mg_unpack(const char *path, size_t *size, time_t *mtime);
const char *mg_unlist(size_t no);

#if MG_ENABLE_PACKED_FS
#else
const char *mg_unpack(const char *path, size_t *size, time_t *mtime) {
  (void) path, (void) size, (void) mtime;
  return NULL;
}
const char *mg_unlist(size_t no) {
  (void) no;
  return NULL;
}
#endif

static int is_dir_prefix(const char *prefix, size_t n, const char *path) {
  // MG_INFO(("[%.*s] [%s] %c", (int) n, prefix, path, path[n]));
  return n < strlen(path) && strncmp(prefix, path, n) == 0 &&
         (n == 0 || path[n] == '/' || path[n - 1] == '/');
}

static int packed_stat(const char *path, size_t *size, time_t *mtime) {
  const char *p;
  size_t i, n = strlen(path);
  if (mg_unpack(path, size, mtime)) return MG_FS_READ;  // Regular file
  // Scan all files. If `path` is a dir prefix for any of them, it's a dir
  for (i = 0; (p = mg_unlist(i)) != NULL; i++) {
    if (is_dir_prefix(path, n, p)) return MG_FS_DIR;
  }
  return 0;
}

static void packed_list(const char *dir, void (*fn)(const char *, void *),
                        void *userdata) {
  char buf[256], tmp[sizeof(buf)];
  const char *path, *begin, *end;
  size_t i, n = strlen(dir);
  tmp[0] = '\0';  // Previously listed entry
  for (i = 0; (path = mg_unlist(i)) != NULL; i++) {
    if (!is_dir_prefix(dir, n, path)) continue;
    begin = &path[n + 1];
    end = strchr(begin, '/');
    if (end == NULL) end = begin + strlen(begin);
    mg_snprintf(buf, sizeof(buf), "%.*s", (int) (end - begin), begin);
    buf[sizeof(buf) - 1] = '\0';
    // If this entry has been already listed, skip
    // NOTE: we're assuming that file list is sorted alphabetically
    if (strcmp(buf, tmp) == 0) continue;
    fn(buf, userdata);  // Not yet listed, call user function
    strcpy(tmp, buf);   // And save this entry as listed
  }
}

static void *packed_open(const char *path, int flags) {
  size_t size = 0;
  const char *data = mg_unpack(path, &size, NULL);
  struct packed_file *fp = NULL;
  if (data == NULL) return NULL;
  if (flags & MG_FS_WRITE) return NULL;
  fp = (struct packed_file *) calloc(1, sizeof(*fp));
  fp->size = size;
  fp->data = data;
  return (void *) fp;
}

static void packed_close(void *fp) {
  if (fp != NULL) free(fp);
}

static size_t packed_read(void *fd, void *buf, size_t len) {
  struct packed_file *fp = (struct packed_file *) fd;
  if (fp->pos + len > fp->size) len = fp->size - fp->pos;
  memcpy(buf, &fp->data[fp->pos], len);
  fp->pos += len;
  return len;
}

static size_t packed_write(void *fd, const void *buf, size_t len) {
  (void) fd, (void) buf, (void) len;
  return 0;
}

static size_t packed_seek(void *fd, size_t offset) {
  struct packed_file *fp = (struct packed_file *) fd;
  fp->pos = offset;
  if (fp->pos > fp->size) fp->pos = fp->size;
  return fp->pos;
}

static bool packed_rename(const char *from, const char *to) {
  (void) from, (void) to;
  return false;
}

static bool packed_remove(const char *path) {
  (void) path;
  return false;
}

static bool packed_mkdir(const char *path) {
  (void) path;
  return false;
}

struct mg_fs mg_fs_packed = {
    packed_stat,  packed_list, packed_open,   packed_close,  packed_read,
    packed_write, packed_seek, packed_rename, packed_remove, packed_mkdir};

#ifdef MG_ENABLE_LINES
#line 1 "src/fs_posix.c"
#endif


#if MG_ENABLE_FILE

#ifndef MG_STAT_STRUCT
#define MG_STAT_STRUCT stat
#endif

#ifndef MG_STAT_FUNC
#define MG_STAT_FUNC stat
#endif

static int p_stat(const char *path, size_t *size, time_t *mtime) {
#if !defined(S_ISDIR)
  MG_ERROR(("stat() API is not supported. %p %p %p", path, size, mtime));
  return 0;
#else
#if MG_ARCH == MG_ARCH_WIN32
  struct _stati64 st;
  wchar_t tmp[PATH_MAX];
  MultiByteToWideChar(CP_UTF8, 0, path, -1, tmp, sizeof(tmp) / sizeof(tmp[0]));
  if (_wstati64(tmp, &st) != 0) return 0;
#else
  struct MG_STAT_STRUCT st;
  if (MG_STAT_FUNC(path, &st) != 0) return 0;
#endif
  if (size) *size = (size_t) st.st_size;
  if (mtime) *mtime = st.st_mtime;
  return MG_FS_READ | MG_FS_WRITE | (S_ISDIR(st.st_mode) ? MG_FS_DIR : 0);
#endif
}

#if MG_ARCH == MG_ARCH_WIN32
struct dirent {
  char d_name[MAX_PATH];
};

typedef struct win32_dir {
  HANDLE handle;
  WIN32_FIND_DATAW info;
  struct dirent result;
} DIR;

int gettimeofday(struct timeval *tv, void *tz) {
  FILETIME ft;
  unsigned __int64 tmpres = 0;

  if (tv != NULL) {
    GetSystemTimeAsFileTime(&ft);
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
    tmpres /= 10;  // convert into microseconds
    tmpres -= (int64_t) 11644473600000000;
    tv->tv_sec = (long) (tmpres / 1000000UL);
    tv->tv_usec = (long) (tmpres % 1000000UL);
  }
  (void) tz;
  return 0;
}

static int to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len) {
  int ret;
  char buf[MAX_PATH * 2], buf2[MAX_PATH * 2], *p;
  strncpy(buf, path, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  // Trim trailing slashes. Leave backslash for paths like "X:\"
  p = buf + strlen(buf) - 1;
  while (p > buf && p[-1] != ':' && (p[0] == '\\' || p[0] == '/')) *p-- = '\0';
  memset(wbuf, 0, wbuf_len * sizeof(wchar_t));
  ret = MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);
  // Convert back to Unicode. If doubly-converted string does not match the
  // original, something is fishy, reject.
  WideCharToMultiByte(CP_UTF8, 0, wbuf, (int) wbuf_len, buf2, sizeof(buf2),
                      NULL, NULL);
  if (strcmp(buf, buf2) != 0) {
    wbuf[0] = L'\0';
    ret = 0;
  }
  return ret;
}

DIR *opendir(const char *name) {
  DIR *d = NULL;
  wchar_t wpath[MAX_PATH];
  DWORD attrs;

  if (name == NULL) {
    SetLastError(ERROR_BAD_ARGUMENTS);
  } else if ((d = (DIR *) calloc(1, sizeof(*d))) == NULL) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  } else {
    to_wchar(name, wpath, sizeof(wpath) / sizeof(wpath[0]));
    attrs = GetFileAttributesW(wpath);
    if (attrs != 0Xffffffff && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
      (void) wcscat(wpath, L"\\*");
      d->handle = FindFirstFileW(wpath, &d->info);
      d->result.d_name[0] = '\0';
    } else {
      free(d);
      d = NULL;
    }
  }
  return d;
}

int closedir(DIR *d) {
  int result = 0;
  if (d != NULL) {
    if (d->handle != INVALID_HANDLE_VALUE)
      result = FindClose(d->handle) ? 0 : -1;
    free(d);
  } else {
    result = -1;
    SetLastError(ERROR_BAD_ARGUMENTS);
  }
  return result;
}

struct dirent *readdir(DIR *d) {
  struct dirent *result = NULL;
  if (d != NULL) {
    memset(&d->result, 0, sizeof(d->result));
    if (d->handle != INVALID_HANDLE_VALUE) {
      result = &d->result;
      WideCharToMultiByte(CP_UTF8, 0, d->info.cFileName, -1, result->d_name,
                          sizeof(result->d_name), NULL, NULL);
      if (!FindNextFileW(d->handle, &d->info)) {
        FindClose(d->handle);
        d->handle = INVALID_HANDLE_VALUE;
      }
    } else {
      SetLastError(ERROR_FILE_NOT_FOUND);
    }
  } else {
    SetLastError(ERROR_BAD_ARGUMENTS);
  }
  return result;
}
#endif

static void p_list(const char *dir, void (*fn)(const char *, void *),
                   void *userdata) {
#if MG_ENABLE_DIRLIST
  struct dirent *dp;
  DIR *dirp;
  if ((dirp = (opendir(dir))) == NULL) return;
  while ((dp = readdir(dirp)) != NULL) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;
    fn(dp->d_name, userdata);
  }
  closedir(dirp);
#else
  (void) dir, (void) fn, (void) userdata;
#endif
}

static void *p_open(const char *path, int flags) {
  const char *mode = flags == MG_FS_READ ? "rb" : "a+b";
#if MG_ARCH == MG_ARCH_WIN32
  wchar_t b1[PATH_MAX], b2[10];
  MultiByteToWideChar(CP_UTF8, 0, path, -1, b1, sizeof(b1) / sizeof(b1[0]));
  MultiByteToWideChar(CP_UTF8, 0, mode, -1, b2, sizeof(b2) / sizeof(b2[0]));
  return (void *) _wfopen(b1, b2);
#else
  return (void *) fopen(path, mode);
#endif
}

static void p_close(void *fp) {
  fclose((FILE *) fp);
}

static size_t p_read(void *fp, void *buf, size_t len) {
  return fread(buf, 1, len, (FILE *) fp);
}

static size_t p_write(void *fp, const void *buf, size_t len) {
  return fwrite(buf, 1, len, (FILE *) fp);
}

static size_t p_seek(void *fp, size_t offset) {
#if (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64) ||  \
    (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) || \
    (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
  if (fseeko((FILE *) fp, (off_t) offset, SEEK_SET) != 0) (void) 0;
#else
  if (fseek((FILE *) fp, (long) offset, SEEK_SET) != 0) (void) 0;
#endif
  return (size_t) ftell((FILE *) fp);
}

static bool p_rename(const char *from, const char *to) {
  return rename(from, to) == 0;
}

static bool p_remove(const char *path) {
  return remove(path) == 0;
}

static bool p_mkdir(const char *path) {
  return mkdir(path, 0775) == 0;
}

#else

static int p_stat(const char *path, size_t *size, time_t *mtime) {
  (void) path, (void) size, (void) mtime;
  return 0;
}
static void p_list(const char *path, void (*fn)(const char *, void *),
                   void *userdata) {
  (void) path, (void) fn, (void) userdata;
}
static void *p_open(const char *path, int flags) {
  (void) path, (void) flags;
  return NULL;
}
static void p_close(void *fp) {
  (void) fp;
}
static size_t p_read(void *fd, void *buf, size_t len) {
  (void) fd, (void) buf, (void) len;
  return 0;
}
static size_t p_write(void *fd, const void *buf, size_t len) {
  (void) fd, (void) buf, (void) len;
  return 0;
}
static size_t p_seek(void *fd, size_t offset) {
  (void) fd, (void) offset;
  return (size_t) ~0;
}
static bool p_rename(const char *from, const char *to) {
  (void) from, (void) to;
  return false;
}
static bool p_remove(const char *path) {
  (void) path;
  return false;
}
static bool p_mkdir(const char *path) {
  (void) path;
  return false;
}
#endif

struct mg_fs mg_fs_posix = {p_stat,  p_list, p_open,   p_close,  p_read,
                            p_write, p_seek, p_rename, p_remove, p_mkdir};

#ifdef MG_ENABLE_LINES
#line 1 "src/http.c"
#endif










// Multipart POST example:
// --xyz
// Content-Disposition: form-data; name="val"
//
// abcdef
// --xyz
// Content-Disposition: form-data; name="foo"; filename="a.txt"
// Content-Type: text/plain
//
// hello world
//
// --xyz--
size_t mg_http_next_multipart(struct mg_str body, size_t ofs,
                              struct mg_http_part *part) {
  struct mg_str cd = mg_str_n("Content-Disposition", 19);
  const char *s = body.ptr;
  size_t b = ofs, h1, h2, b1, b2, max = body.len;

  // Init part params
  if (part != NULL) part->name = part->filename = part->body = mg_str_n(0, 0);

  // Skip boundary
  while (b + 2 < max && s[b] != '\r' && s[b + 1] != '\n') b++;
  if (b <= ofs || b + 2 >= max) return 0;
  // MG_INFO(("B: %zu %zu [%.*s]", ofs, b - ofs, (int) (b - ofs), s));

  // Skip headers
  h1 = h2 = b + 2;
  for (;;) {
    while (h2 + 2 < max && s[h2] != '\r' && s[h2 + 1] != '\n') h2++;
    if (h2 == h1) break;
    if (h2 + 2 >= max) return 0;
    // MG_INFO(("Header: [%.*s]", (int) (h2 - h1), &s[h1]));
    if (part != NULL && h1 + cd.len + 2 < h2 && s[h1 + cd.len] == ':' &&
        mg_ncasecmp(&s[h1], cd.ptr, cd.len) == 0) {
      struct mg_str v = mg_str_n(&s[h1 + cd.len + 2], h2 - (h1 + cd.len + 2));
      part->name = mg_http_get_header_var(v, mg_str_n("name", 4));
      part->filename = mg_http_get_header_var(v, mg_str_n("filename", 8));
    }
    h1 = h2 = h2 + 2;
  }
  b1 = b2 = h2 + 2;
  while (b2 + 2 + (b - ofs) + 2 < max && !(s[b2] == '\r' && s[b2 + 1] == '\n' &&
                                           memcmp(&s[b2 + 2], s, b - ofs) == 0))
    b2++;

  if (b2 + 2 >= max) return 0;
  if (part != NULL) part->body = mg_str_n(&s[b1], b2 - b1);
  // MG_INFO(("Body: [%.*s]", (int) (b2 - b1), &s[b1]));
  return b2 + 2;
}

void mg_http_bauth(struct mg_connection *c, const char *user,
                   const char *pass) {
  struct mg_str u = mg_str(user), p = mg_str(pass);
  size_t need = c->send.len + 36 + (u.len + p.len) * 2;
  if (c->send.size < need) mg_iobuf_resize(&c->send, need);
  if (c->send.size >= need) {
    int i, n = 0;
    char *buf = (char *) &c->send.buf[c->send.len + 21];
    memcpy(&buf[-21], "Authorization: Basic ", 21);  // DON'T use mg_send!
    for (i = 0; i < (int) u.len; i++) {
      n = mg_base64_update(((unsigned char *) u.ptr)[i], buf, n);
    }
    if (p.len > 0) {
      n = mg_base64_update(':', buf, n);
      for (i = 0; i < (int) p.len; i++) {
        n = mg_base64_update(((unsigned char *) p.ptr)[i], buf, n);
      }
    }
    n = mg_base64_final(buf, n);
    c->send.len += 21 + (size_t) n + 2;
    memcpy(&c->send.buf[c->send.len - 2], "\r\n", 2);
  } else {
    MG_ERROR(("%lu %s cannot resize iobuf %d->%d ", c->id, c->label,
              (int) c->send.size, (int) need));
  }
}

int mg_http_get_var(const struct mg_str *buf, const char *name, char *dst,
                    size_t dst_len) {
  const char *p, *e, *s;
  size_t name_len;
  int len;

  if (dst == NULL || dst_len == 0) {
    len = -2;  // Bad destination
  } else if (buf->ptr == NULL || name == NULL || buf->len == 0) {
    len = -1;  // Bad source
    dst[0] = '\0';
  } else {
    name_len = strlen(name);
    e = buf->ptr + buf->len;
    len = -4;  // Name does not exist
    dst[0] = '\0';
    for (p = buf->ptr; p + name_len < e; p++) {
      if ((p == buf->ptr || p[-1] == '&') && p[name_len] == '=' &&
          !mg_ncasecmp(name, p, name_len)) {
        p += name_len + 1;
        s = (const char *) memchr(p, '&', (size_t) (e - p));
        if (s == NULL) s = e;
        len = mg_url_decode(p, (size_t) (s - p), dst, dst_len, 1);
        if (len < 0) len = -3;  // Failed to decode
        break;
      }
    }
  }
  return len;
}

static bool isx(int c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

int mg_url_decode(const char *src, size_t src_len, char *dst, size_t dst_len,
                  int is_form_url_encoded) {
  size_t i, j;
  for (i = j = 0; i < src_len && j + 1 < dst_len; i++, j++) {
    if (src[i] == '%') {
      // Use `i + 2 < src_len`, not `i < src_len - 2`, note small src_len
      if (i + 2 < src_len && isx(src[i + 1]) && isx(src[i + 2])) {
        mg_unhex(src + i + 1, 2, (uint8_t *) &dst[j]);
        i += 2;
      } else {
        return -1;
      }
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }
  if (j < dst_len) dst[j] = '\0';  // Null-terminate the destination
  return i >= src_len && j < dst_len ? (int) j : -1;
}

static bool isok(uint8_t c) {
  return c == '\n' || c == '\r' || c >= ' ';
}

int mg_http_get_request_len(const unsigned char *buf, size_t buf_len) {
  size_t i;
  for (i = 0; i < buf_len; i++) {
    if (!isok(buf[i])) return -1;
    if ((i > 0 && buf[i] == '\n' && buf[i - 1] == '\n') ||
        (i > 3 && buf[i] == '\n' && buf[i - 1] == '\r' && buf[i - 2] == '\n'))
      return (int) i + 1;
  }
  return 0;
}

static const char *skip(const char *s, const char *e, const char *d,
                        struct mg_str *v) {
  v->ptr = s;
  while (s < e && *s != '\n' && strchr(d, *s) == NULL) s++;
  v->len = (size_t) (s - v->ptr);
  while (s < e && strchr(d, *s) != NULL) s++;
  return s;
}

struct mg_str *mg_http_get_header(struct mg_http_message *h, const char *name) {
  size_t i, n = strlen(name), max = sizeof(h->headers) / sizeof(h->headers[0]);
  for (i = 0; i < max && h->headers[i].name.len > 0; i++) {
    struct mg_str *k = &h->headers[i].name, *v = &h->headers[i].value;
    if (n == k->len && mg_ncasecmp(k->ptr, name, n) == 0) return v;
  }
  return NULL;
}

static void mg_http_parse_headers(const char *s, const char *end,
                                  struct mg_http_header *h, int max_headers) {
  int i;
  for (i = 0; i < max_headers; i++) {
    struct mg_str k, v, tmp;
    const char *he = skip(s, end, "\n", &tmp);
    s = skip(s, he, ": \r\n", &k);
    s = skip(s, he, "\r\n", &v);
    if (k.len == tmp.len) continue;
    while (v.len > 0 && v.ptr[v.len - 1] == ' ') v.len--;  // Trim spaces
    if (k.len == 0) break;
    // MG_INFO(("--HH [%.*s] [%.*s] [%.*s]", (int) tmp.len - 1, tmp.ptr,
    //(int) k.len, k.ptr, (int) v.len, v.ptr));
    h[i].name = k;
    h[i].value = v;
  }
}

int mg_http_parse(const char *s, size_t len, struct mg_http_message *hm) {
  int is_response, req_len = mg_http_get_request_len((unsigned char *) s, len);
  const char *end = s + req_len, *qs;
  struct mg_str *cl;

  memset(hm, 0, sizeof(*hm));
  if (req_len <= 0) return req_len;

  hm->message.ptr = hm->head.ptr = s;
  hm->body.ptr = end;
  hm->head.len = (size_t) req_len;
  hm->chunk.ptr = end;
  hm->message.len = hm->body.len = (size_t) ~0;  // Set body length to infinite

  // Parse request line
  s = skip(s, end, " ", &hm->method);
  s = skip(s, end, " ", &hm->uri);
  s = skip(s, end, "\r\n", &hm->proto);

  // Sanity check. Allow protocol/reason to be empty
  if (hm->method.len == 0 || hm->uri.len == 0) return -1;

  // If URI contains '?' character, setup query string
  if ((qs = (const char *) memchr(hm->uri.ptr, '?', hm->uri.len)) != NULL) {
    hm->query.ptr = qs + 1;
    hm->query.len = (size_t) (&hm->uri.ptr[hm->uri.len] - (qs + 1));
    hm->uri.len = (size_t) (qs - hm->uri.ptr);
  }

  mg_http_parse_headers(s, end, hm->headers,
                        sizeof(hm->headers) / sizeof(hm->headers[0]));
  if ((cl = mg_http_get_header(hm, "Content-Length")) != NULL) {
    hm->body.len = (size_t) mg_to64(*cl);
    hm->message.len = (size_t) req_len + hm->body.len;
  }

  // mg_http_parse() is used to parse both HTTP requests and HTTP
  // responses. If HTTP response does not have Content-Length set, then
  // body is read until socket is closed, i.e. body.len is infinite (~0).
  //
  // For HTTP requests though, according to
  // http://tools.ietf.org/html/rfc7231#section-8.1.3,
  // only POST and PUT methods have defined body semantics.
  // Therefore, if Content-Length is not specified and methods are
  // not one of PUT or POST, set body length to 0.
  //
  // So, if it is HTTP request, and Content-Length is not set,
  // and method is not (PUT or POST) then reset body length to zero.
  is_response = mg_ncasecmp(hm->method.ptr, "HTTP/", 5) == 0;
  if (hm->body.len == (size_t) ~0 && !is_response &&
      mg_vcasecmp(&hm->method, "PUT") != 0 &&
      mg_vcasecmp(&hm->method, "POST") != 0) {
    hm->body.len = 0;
    hm->message.len = (size_t) req_len;
  }

  // The 204 (No content) responses also have 0 body length
  if (hm->body.len == (size_t) ~0 && is_response &&
      mg_vcasecmp(&hm->uri, "204") == 0) {
    hm->body.len = 0;
    hm->message.len = (size_t) req_len;
  }

  return req_len;
}

static void mg_http_vprintf_chunk(struct mg_connection *c, const char *fmt,
                                  va_list ap) {
  char mem[256], *buf = mem;
  size_t len = mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  mg_printf(c, "%lx\r\n", (unsigned long) len);
  mg_send(c, buf, len > 0 ? (size_t) len : 0);
  mg_send(c, "\r\n", 2);
  if (buf != mem) free(buf);
}

void mg_http_printf_chunk(struct mg_connection *c, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  mg_http_vprintf_chunk(c, fmt, ap);
  va_end(ap);
}

void mg_http_write_chunk(struct mg_connection *c, const char *buf, size_t len) {
  mg_printf(c, "%lx\r\n", (unsigned long) len);
  mg_send(c, buf, len);
  mg_send(c, "\r\n", 2);
}

// clang-format off
static const char *mg_http_status_code_str(int status_code) {
  switch (status_code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Payload Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 418: return "I'm a teapot";
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Entity";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 444: return "Connection Closed Without Response";
    case 451: return "Unavailable For Legal Reasons";
    case 499: return "Client Closed Request";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    case 510: return "Not Extended";
    case 511: return "Network Authentication Required";
    case 599: return "Network Connect Timeout Error";
    default: return "OK";
  }
}
// clang-format on

void mg_http_reply(struct mg_connection *c, int code, const char *headers,
                   const char *fmt, ...) {
  char mem[256], *buf = mem;
  va_list ap;
  size_t len;
  va_start(ap, fmt);
  len = mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  va_end(ap);
  mg_printf(c, "HTTP/1.1 %d %s\r\n%sContent-Length: %d\r\n\r\n", code,
            mg_http_status_code_str(code), headers == NULL ? "" : headers, len);
  mg_send(c, buf, len > 0 ? len : 0);
  if (buf != mem) free(buf);
}

static void http_cb(struct mg_connection *, int, void *, void *);
static void restore_http_cb(struct mg_connection *c) {
  mg_fs_close((struct mg_fd *) c->pfn_data);
  c->pfn_data = NULL;
  c->pfn = http_cb;
}

char *mg_http_etag(char *buf, size_t len, size_t size, time_t mtime);
char *mg_http_etag(char *buf, size_t len, size_t size, time_t mtime) {
  mg_snprintf(buf, len, "\"%lld.%lld\"", (int64_t) mtime, (int64_t) size);
  return buf;
}

static void static_cb(struct mg_connection *c, int ev, void *ev_data,
                      void *fn_data) {
  if (ev == MG_EV_WRITE || ev == MG_EV_POLL) {
    struct mg_fd *fd = (struct mg_fd *) fn_data;
    // Read to send IO buffer directly, avoid extra on-stack buffer
    size_t n, max = 2 * MG_IO_SIZE;
    if (c->send.size < max) mg_iobuf_resize(&c->send, max);
    if (c->send.len >= c->send.size) return;  // Rate limit
    n = fd->fs->rd(fd->fd, c->send.buf + c->send.len,
                   c->send.size - c->send.len);
    c->send.len += n;
    if (n == 0) restore_http_cb(c);
  } else if (ev == MG_EV_CLOSE) {
    restore_http_cb(c);
  }
  (void) ev_data;
}

static struct mg_str guess_content_type(struct mg_str path, const char *extra) {
  struct mg_str k, v, s = mg_str(extra);
  size_t i = 0;

  // clang-format off
  struct mg_str tab[] = {
      MG_C_STR("html"), MG_C_STR("text/html; charset=utf-8"),
      MG_C_STR("htm"), MG_C_STR("text/html; charset=utf-8"),
      MG_C_STR("css"), MG_C_STR("text/css; charset=utf-8"),
      MG_C_STR("js"), MG_C_STR("text/javascript; charset=utf-8"),
      MG_C_STR("gif"), MG_C_STR("image/gif"),
      MG_C_STR("png"), MG_C_STR("image/png"),
      MG_C_STR("jpg"), MG_C_STR("image/jpeg"),
      MG_C_STR("jpeg"), MG_C_STR("image/jpeg"),
      MG_C_STR("woff"), MG_C_STR("font/woff"),
      MG_C_STR("ttf"), MG_C_STR("font/ttf"),
      MG_C_STR("svg"), MG_C_STR("image/svg+xml"),
      MG_C_STR("txt"), MG_C_STR("text/plain; charset=utf-8"),
      MG_C_STR("avi"), MG_C_STR("video/x-msvideo"),
      MG_C_STR("csv"), MG_C_STR("text/csv"),
      MG_C_STR("doc"), MG_C_STR("application/msword"),
      MG_C_STR("exe"), MG_C_STR("application/octet-stream"),
      MG_C_STR("gz"), MG_C_STR("application/gzip"),
      MG_C_STR("ico"), MG_C_STR("image/x-icon"),
      MG_C_STR("json"), MG_C_STR("application/json"),
      MG_C_STR("mov"), MG_C_STR("video/quicktime"),
      MG_C_STR("mp3"), MG_C_STR("audio/mpeg"),
      MG_C_STR("mp4"), MG_C_STR("video/mp4"),
      MG_C_STR("mpeg"), MG_C_STR("video/mpeg"),
      MG_C_STR("pdf"), MG_C_STR("application/pdf"),
      MG_C_STR("shtml"), MG_C_STR("text/html; charset=utf-8"),
      MG_C_STR("tgz"), MG_C_STR("application/tar-gz"),
      MG_C_STR("wav"), MG_C_STR("audio/wav"),
      MG_C_STR("webp"), MG_C_STR("image/webp"),
      MG_C_STR("zip"), MG_C_STR("application/zip"),
      MG_C_STR("3gp"), MG_C_STR("video/3gpp"),
      {0, 0},
  };
  // clang-format on

  // Shrink path to its extension only
  while (i < path.len && path.ptr[path.len - i - 1] != '.') i++;
  path.ptr += path.len - i;
  path.len = i;

  // Process user-provided mime type overrides, if any
  while (mg_commalist(&s, &k, &v)) {
    if (mg_strcmp(path, k) == 0) return v;
  }

  // Process built-in mime types
  for (i = 0; tab[i].ptr != NULL; i += 2) {
    if (mg_strcmp(path, tab[i]) == 0) return tab[i + 1];
  }

  return mg_str("text/plain; charset=utf-8");
}

static int getrange(struct mg_str *s, int64_t *a, int64_t *b) {
  size_t i, numparsed = 0;
  // MG_INFO(("%.*s", (int) s->len, s->ptr));
  for (i = 0; i + 6 < s->len; i++) {
    if (memcmp(&s->ptr[i], "bytes=", 6) == 0) {
      struct mg_str p = mg_str_n(s->ptr + i + 6, s->len - i - 6);
      if (p.len > 0 && p.ptr[0] >= '0' && p.ptr[0] <= '9') numparsed++;
      *a = mg_to64(p);
      // MG_INFO(("PPP [%.*s] %d", (int) p.len, p.ptr, numparsed));
      while (p.len && p.ptr[0] >= '0' && p.ptr[0] <= '9') p.ptr++, p.len--;
      if (p.len && p.ptr[0] == '-') p.ptr++, p.len--;
      *b = mg_to64(p);
      if (p.len > 0 && p.ptr[0] >= '0' && p.ptr[0] <= '9') numparsed++;
      // MG_INFO(("PPP [%.*s] %d", (int) p.len, p.ptr, numparsed));
      break;
    }
  }
  return (int) numparsed;
}

void mg_http_serve_file(struct mg_connection *c, struct mg_http_message *hm,
                        const char *path, struct mg_http_serve_opts *opts) {
  char etag[64];
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct mg_fd *fd = mg_fs_open(fs, path, MG_FS_READ);
  size_t size = 0;
  time_t mtime = 0;
  struct mg_str *inm = NULL;

  if (fd == NULL || fs->st(path, &size, &mtime) == 0) {
    MG_DEBUG(("404 [%s] %p", path, (void *) fd));
    mg_http_reply(c, 404, "", "%s", "Not found\n");
    mg_fs_close(fd);
    // NOTE: mg_http_etag() call should go first!
  } else if (mg_http_etag(etag, sizeof(etag), size, mtime) != NULL &&
             (inm = mg_http_get_header(hm, "If-None-Match")) != NULL &&
             mg_vcasecmp(inm, etag) == 0) {
    mg_fs_close(fd);
    mg_printf(c, "HTTP/1.1 304 Not Modified\r\nContent-Length: 0\r\n\r\n");
  } else {
    int n, status = 200;
    char range[100] = "";
    int64_t r1 = 0, r2 = 0, cl = (int64_t) size;
    struct mg_str mime = guess_content_type(mg_str(path), opts->mime_types);

    // Handle Range header
    struct mg_str *rh = mg_http_get_header(hm, "Range");
    if (rh != NULL && (n = getrange(rh, &r1, &r2)) > 0 && r1 >= 0 && r2 >= 0) {
      // If range is specified like "400-", set second limit to content len
      if (n == 1) r2 = cl - 1;
      if (r1 > r2 || r2 >= cl) {
        status = 416;
        cl = 0;
        mg_snprintf(range, sizeof(range), "Content-Range: bytes */%lld\r\n",
                    (int64_t) size);
      } else {
        status = 206;
        cl = r2 - r1 + 1;
        mg_snprintf(range, sizeof(range),
                    "Content-Range: bytes %lld-%lld/%lld\r\n", r1, r1 + cl - 1,
                    (int64_t) size);
        fs->sk(fd->fd, (size_t) r1);
      }
    }
    mg_printf(c,
              "HTTP/1.1 %d %s\r\n"
              "Content-Type: %.*s\r\n"
              "Etag: %s\r\n"
              "Content-Length: %llu\r\n"
              "%s%s\r\n",
              status, mg_http_status_code_str(status), (int) mime.len, mime.ptr,
              etag, cl, range, opts->extra_headers ? opts->extra_headers : "");
    if (mg_vcasecmp(&hm->method, "HEAD") == 0) {
      c->is_draining = 1;
      mg_fs_close(fd);
    } else {
      c->pfn = static_cb;
      c->pfn_data = fd;
    }
  }
}

struct printdirentrydata {
  struct mg_connection *c;
  struct mg_http_message *hm;
  struct mg_http_serve_opts *opts;
  const char *dir;
};

static void printdirentry(const char *name, void *userdata) {
  struct printdirentrydata *d = (struct printdirentrydata *) userdata;
  struct mg_fs *fs = d->opts->fs == NULL ? &mg_fs_posix : d->opts->fs;
  size_t size = 0;
  time_t t = 0;
  char path[MG_PATH_MAX], sz[40], mod[40];
  int flags, n = 0;

  // MG_DEBUG(("[%s] [%s]", d->dir, name));
  if (mg_snprintf(path, sizeof(path), "%s%c%s", d->dir, '/', name) >
      sizeof(path)) {
    MG_ERROR(("%s truncated", name));
  } else if ((flags = fs->st(path, &size, &t)) == 0) {
    MG_ERROR(("%lu stat(%s): %d", d->c->id, path, errno));
  } else {
    const char *slash = flags & MG_FS_DIR ? "/" : "";
    if (flags & MG_FS_DIR) {
      mg_snprintf(sz, sizeof(sz), "%s", "[DIR]");
    } else {
      mg_snprintf(sz, sizeof(sz), "%lld", (uint64_t) size);
    }
    mg_snprintf(mod, sizeof(mod), "%ld", (unsigned long) t);
    n = (int) mg_url_encode(name, strlen(name), path, sizeof(path));
    mg_printf(d->c,
              "  <tr><td><a href=\"%.*s%s\">%s%s</a></td>"
              "<td name=%lu>%s</td><td name=%lld>%s</td></tr>\n",
              n, path, slash, name, slash, (unsigned long) t, mod,
              flags & MG_FS_DIR ? (int64_t) -1 : (int64_t) size, sz);
  }
}

static void listdir(struct mg_connection *c, struct mg_http_message *hm,
                    struct mg_http_serve_opts *opts, char *dir) {
  static const char *sort_js_code =
      "<script>function srt(tb, sc, so, d) {"
      "var tr = Array.prototype.slice.call(tb.rows, 0),"
      "tr = tr.sort(function (a, b) { var c1 = a.cells[sc], c2 = b.cells[sc],"
      "n1 = c1.getAttribute('name'), n2 = c2.getAttribute('name'), "
      "t1 = a.cells[2].getAttribute('name'), "
      "t2 = b.cells[2].getAttribute('name'); "
      "return so * (t1 < 0 && t2 >= 0 ? -1 : t2 < 0 && t1 >= 0 ? 1 : "
      "n1 ? parseInt(n2) - parseInt(n1) : "
      "c1.textContent.trim().localeCompare(c2.textContent.trim())); });";
  static const char *sort_js_code2 =
      "for (var i = 0; i < tr.length; i++) tb.appendChild(tr[i]); "
      "if (!d) window.location.hash = ('sc=' + sc + '&so=' + so); "
      "};"
      "window.onload = function() {"
      "var tb = document.getElementById('tb');"
      "var m = /sc=([012]).so=(1|-1)/.exec(window.location.hash) || [0, 2, 1];"
      "var sc = m[1], so = m[2]; document.onclick = function(ev) { "
      "var c = ev.target.rel; if (c) {if (c == sc) so *= -1; srt(tb, c, so); "
      "sc = c; ev.preventDefault();}};"
      "srt(tb, sc, so, true);"
      "}"
      "</script>";
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct printdirentrydata d = {c, hm, opts, dir};
  char tmp[10], buf[MG_PATH_MAX];
  size_t off, n;
  int len = mg_url_decode(hm->uri.ptr, hm->uri.len, buf, sizeof(buf), 0);
  struct mg_str uri = len > 0 ? mg_str_n(buf, (size_t) len) : hm->uri;

  mg_printf(c,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "%s"
            "Content-Length:         \r\n\r\n",
            opts->extra_headers == NULL ? "" : opts->extra_headers);
  off = c->send.len;  // Start of body
  mg_printf(c,
            "<!DOCTYPE html><html><head><title>Index of %.*s</title>%s%s"
            "<style>th,td {text-align: left; padding-right: 1em; "
            "font-family: monospace; }</style></head>"
            "<body><h1>Index of %.*s</h1><table cellpadding=\"0\"><thead>"
            "<tr><th><a href=\"#\" rel=\"0\">Name</a></th><th>"
            "<a href=\"#\" rel=\"1\">Modified</a></th>"
            "<th><a href=\"#\" rel=\"2\">Size</a></th></tr>"
            "<tr><td colspan=\"3\"><hr></td></tr>"
            "</thead>"
            "<tbody id=\"tb\">\n",
            (int) uri.len, uri.ptr, sort_js_code, sort_js_code2, (int) uri.len,
            uri.ptr);
  mg_printf(c, "%s",
            "  <tr><td><a href=\"..\">..</a></td>"
            "<td name=-1></td><td name=-1>[DIR]</td></tr>\n");

  fs->ls(dir, printdirentry, &d);
  mg_printf(c,
            "</tbody><tfoot><tr><td colspan=\"3\"><hr></td></tr></tfoot>"
            "</table><address>Mongoose v.%s</address></body></html>\n",
            MG_VERSION);
  n = mg_snprintf(tmp, sizeof(tmp), "%lu", (unsigned long) (c->send.len - off));
  if (n > sizeof(tmp)) n = 0;
  memcpy(c->send.buf + off - 10, tmp, n);  // Set content length
}

static void remove_double_dots(char *s) {
  char *p = s;
  while (*s != '\0') {
    *p++ = *s++;
    if (s[-1] == '/' || s[-1] == '\\') {
      while (s[0] != '\0') {
        if (s[0] == '/' || s[0] == '\\') {
          s++;
        } else if (s[0] == '.' && s[1] == '.' &&
                   (s[2] == '/' || s[2] == '\\')) {
          s += 2;
        } else {
          break;
        }
      }
    }
  }
  *p = '\0';
}

// Resolve requested file into `path` and return its fs->st() result
static int uri_to_path2(struct mg_connection *c, struct mg_http_message *hm,
                        struct mg_fs *fs, struct mg_str url, struct mg_str dir,
                        char *path, size_t path_size) {
  int flags = 0, tmp;
  // Append URI to the root_dir, and sanitize it
  size_t n = mg_snprintf(path, path_size, "%.*s", (int) dir.len, dir.ptr);
  if (n > path_size) n = path_size;
  path[path_size - 1] = '\0';
  if ((fs->st(path, NULL, NULL) & MG_FS_DIR) == 0) {
    mg_http_reply(c, 400, "", "Invalid web root [%.*s]\n", (int) dir.len,
                  dir.ptr);
  } else {
    if (n + 2 < path_size) path[n++] = '/', path[n] = '\0';
    mg_url_decode(hm->uri.ptr + url.len, hm->uri.len - url.len, path + n,
                  path_size - n, 0);
    path[path_size - 1] = '\0';  // Double-check
    remove_double_dots(path);
    n = strlen(path);
    MG_VERBOSE(("%lu %.*s -> %s", c->id, (int) hm->uri.len, hm->uri.ptr, path));
    while (n > 0 && path[n - 1] == '/') path[--n] = 0;  // Trim trailing slashes
    flags = fs->st(path, NULL, NULL);                   // Does it exist?
    if (flags == 0) {
      mg_http_reply(c, 404, "", "Not found\n");  // Does not exist, doh
    } else if ((flags & MG_FS_DIR) && hm->uri.len > 0 &&
               hm->uri.ptr[hm->uri.len - 1] != '/') {
      mg_printf(c,
                "HTTP/1.1 301 Moved\r\n"
                "Location: %.*s/\r\n"
                "Content-Length: 0\r\n"
                "\r\n",
                (int) hm->uri.len, hm->uri.ptr);
      flags = 0;
    } else if (flags & MG_FS_DIR) {
      if (((mg_snprintf(path + n, path_size - n, "/" MG_HTTP_INDEX) > 0 &&
            (tmp = fs->st(path, NULL, NULL)) != 0) ||
           (mg_snprintf(path + n, path_size - n, "/index.shtml") > 0 &&
            (tmp = fs->st(path, NULL, NULL)) != 0))) {
        flags = tmp;
      } else {
        path[n] = '\0';  // Remove appended index file name
      }
    }
  }
  return flags;
}

static int uri_to_path(struct mg_connection *c, struct mg_http_message *hm,
                       struct mg_http_serve_opts *opts, char *path,
                       size_t path_size) {
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct mg_str k, v, s = mg_str(opts->root_dir), u = {0, 0}, p = {0, 0};
  while (mg_commalist(&s, &k, &v)) {
    if (v.len == 0) v = k, k = mg_str("/");
    if (hm->uri.len < k.len) continue;
    if (mg_strcmp(k, mg_str_n(hm->uri.ptr, k.len)) != 0) continue;
    u = k, p = v;
  }
  return uri_to_path2(c, hm, fs, u, p, path, path_size);
}

void mg_http_serve_dir(struct mg_connection *c, struct mg_http_message *hm,
                       struct mg_http_serve_opts *opts) {
  char path[MG_PATH_MAX] = "";
  const char *sp = opts->ssi_pattern;
  int flags = uri_to_path(c, hm, opts, path, sizeof(path));
  if (flags == 0) return;
  MG_VERBOSE(("%.*s %s %d", (int) hm->uri.len, hm->uri.ptr, path, flags));
  if (flags & MG_FS_DIR) {
    listdir(c, hm, opts, path);
  } else if (sp != NULL && mg_globmatch(sp, strlen(sp), path, strlen(path))) {
    mg_http_serve_ssi(c, opts->root_dir, path);
  } else {
    mg_http_serve_file(c, hm, path, opts);
  }
}

static bool mg_is_url_safe(int c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-' || c == '~';
}

size_t mg_url_encode(const char *s, size_t sl, char *buf, size_t len) {
  size_t i, n = 0;
  for (i = 0; i < sl; i++) {
    int c = *(unsigned char *) &s[i];
    if (n + 4 >= len) return 0;
    if (mg_is_url_safe(c)) {
      buf[n++] = s[i];
    } else {
      buf[n++] = '%';
      mg_hex(&s[i], 1, &buf[n]);
      n += 2;
    }
  }
  return n;
}

void mg_http_creds(struct mg_http_message *hm, char *user, size_t userlen,
                   char *pass, size_t passlen) {
  struct mg_str *v = mg_http_get_header(hm, "Authorization");
  user[0] = pass[0] = '\0';
  if (v != NULL && v->len > 6 && memcmp(v->ptr, "Basic ", 6) == 0) {
    char buf[256];
    int n = mg_base64_decode(v->ptr + 6, (int) v->len - 6, buf);
    const char *p = (const char *) memchr(buf, ':', n > 0 ? (size_t) n : 0);
    if (p != NULL) {
      mg_snprintf(user, userlen, "%.*s", (int) (p - buf), buf);
      mg_snprintf(pass, passlen, "%.*s", n - (int) (p - buf) - 1, p + 1);
    }
  } else if (v != NULL && v->len > 7 && memcmp(v->ptr, "Bearer ", 7) == 0) {
    mg_snprintf(pass, passlen, "%.*s", (int) v->len - 7, v->ptr + 7);
  } else if ((v = mg_http_get_header(hm, "Cookie")) != NULL) {
    struct mg_str t = mg_http_get_header_var(*v, mg_str_n("access_token", 12));
    if (t.len > 0) mg_snprintf(pass, passlen, "%.*s", (int) t.len, t.ptr);
  } else {
    mg_http_get_var(&hm->query, "access_token", pass, passlen);
  }
}

static struct mg_str stripquotes(struct mg_str s) {
  return s.len > 1 && s.ptr[0] == '"' && s.ptr[s.len - 1] == '"'
             ? mg_str_n(s.ptr + 1, s.len - 2)
             : s;
}

struct mg_str mg_http_get_header_var(struct mg_str s, struct mg_str v) {
  size_t i;
  for (i = 0; v.len > 0 && i + v.len + 2 < s.len; i++) {
    if (s.ptr[i + v.len] == '=' && memcmp(&s.ptr[i], v.ptr, v.len) == 0) {
      const char *p = &s.ptr[i + v.len + 1], *b = p, *x = &s.ptr[s.len];
      int q = p < x && *p == '"' ? 1 : 0;
      while (p < x &&
             (q ? p == b || *p != '"' : *p != ';' && *p != ' ' && *p != ','))
        p++;
      // MG_INFO(("[%.*s] [%.*s] [%.*s]", (int) s.len, s.ptr, (int) v.len,
      // v.ptr, (int) (p - b), b));
      return stripquotes(mg_str_n(b, (size_t) (p - b + q)));
    }
  }
  return mg_str_n(NULL, 0);
}

bool mg_http_match_uri(const struct mg_http_message *hm, const char *glob) {
  return mg_match(hm->uri, mg_str(glob), NULL);
}

static size_t get_chunk_length(const char *buf, size_t len, size_t *ll) {
  size_t i = 0, n;
  while (i < len && buf[i] != '\r' && i != '\n') i++;
  n = mg_unhexn((char *) buf, i);
  while (i < len && (buf[i] == '\r' || i == '\n')) i++;
  // MG_INFO(("len %zu i %zu n %zu ", len, i, n));
  if (ll != NULL) *ll = i + 1;
  if (i < len && i + n + 2 < len) return i + n + 3;
  return 0;
}

// Walk through all chunks in the chunked body. For each chunk, fire
// an MG_EV_HTTP_CHUNK event.
static void walkchunks(struct mg_connection *c, struct mg_http_message *hm,
                       size_t reqlen) {
  size_t off = 0, bl, ll;
  while (off + reqlen < c->recv.len) {
    char *buf = (char *) &c->recv.buf[reqlen];
    size_t memo = c->recv.len;
    size_t cl = get_chunk_length(&buf[off], memo - reqlen - off, &ll);
    // MG_INFO(("len %zu off %zu cl %zu ll %zu", len, off, cl, ll));
    if (cl == 0) break;
    hm->chunk = mg_str_n(&buf[off + ll], cl < ll + 2 ? 0 : cl - ll - 2);
    mg_call(c, MG_EV_HTTP_CHUNK, hm);
    // Increase offset only if user has not deleted this chunk
    if (memo == c->recv.len) off += cl;
    if (cl <= 5) {
      // Zero chunk - last one. Prepare body - cut off chunk lengths
      off = bl = 0;
      while (off + reqlen < c->recv.len) {
        char *buf2 = (char *) &c->recv.buf[reqlen];
        size_t memo2 = c->recv.len;
        size_t cl2 = get_chunk_length(&buf2[off], memo2 - reqlen - off, &ll);
        size_t n = cl2 < ll + 2 ? 0 : cl2 - ll - 2;
        memmove(buf2 + bl, buf2 + off + ll, n);
        bl += n;
        off += cl2;
        if (cl2 <= 5) break;
      }
      // MG_INFO(("BL->%d del %d off %d", (int) bl, (int) del, (int) off));
      c->recv.len -= off - bl;
      // Set message length to indicate we've received
      // everything, to fire MG_EV_HTTP_MSG
      hm->message.len = bl + reqlen;
      hm->body.len = bl;
      break;
    }
  }
}

static bool mg_is_chunked(struct mg_http_message *hm) {
  struct mg_str needle = mg_str_n("chunked", 7);
  struct mg_str *te = mg_http_get_header(hm, "Transfer-Encoding");
  return te != NULL && mg_strstr(*te, needle) != NULL;
}

void mg_http_delete_chunk(struct mg_connection *c, struct mg_http_message *hm) {
  struct mg_str ch = hm->chunk;
  const char *end = (char *) &c->recv.buf[c->recv.len], *ce;
  bool chunked = mg_is_chunked(hm);
  if (chunked) {
    ch.len += 4, ch.ptr -= 2;  // \r\n before and after the chunk
    while (ch.ptr > hm->body.ptr && *ch.ptr != '\n') ch.ptr--, ch.len++;
  }
  ce = &ch.ptr[ch.len];
  if (ce < end) memmove((void *) ch.ptr, ce, (size_t) (end - ce));
  c->recv.len -= ch.len;
  if (c->pfn_data != NULL) c->pfn_data = (char *) c->pfn_data - ch.len;
}

int mg_http_upload(struct mg_connection *c, struct mg_http_message *hm,
                   struct mg_fs *fs, const char *dir) {
  char offset[40] = "", name[200] = "", path[256];
  int res = 0;
  mg_http_get_var(&hm->query, "offset", offset, sizeof(offset));
  mg_http_get_var(&hm->query, "name", name, sizeof(name));
  if (name[0] == '\0') {
    mg_http_reply(c, 400, "", "%s", "name required");
    res = -1;
  } else if (hm->body.len > 0) {
    struct mg_fd *fd;
    long oft = strtol(offset, NULL, 0);
    mg_snprintf(path, sizeof(path), "%s%c%s", dir, MG_DIRSEP, name);
    remove_double_dots(path);
    MG_DEBUG(("%d bytes @ %ld [%s]", (int) hm->body.len, oft, path));
    if (oft == 0) fs->rm(path);
    if ((fd = mg_fs_open(fs, path, MG_FS_WRITE)) == NULL) {
      mg_http_reply(c, 400, "", "open(%s): %d", path, errno);
      res = -2;
    } else {
      res = (int) fs->wr(fd->fd, hm->body.ptr, hm->body.len);
      mg_fs_close(fd);
      mg_http_reply(c, 200, "", "%d", res);
    }
  }
  return res;
}

int mg_http_status(const struct mg_http_message *hm) {
  return atoi(hm->uri.ptr);
}

static void http_cb(struct mg_connection *c, int ev, void *evd, void *fnd) {
  if (ev == MG_EV_READ || ev == MG_EV_CLOSE) {
    struct mg_http_message hm;
    while (c->recv.buf != NULL && c->recv.len > 0) {
      int n = mg_http_parse((char *) c->recv.buf, c->recv.len, &hm);
      bool is_chunked = n > 0 && mg_is_chunked(&hm);
      if (ev == MG_EV_CLOSE) {
        hm.message.len = c->recv.len;
        hm.body.len = hm.message.len - (size_t) (hm.body.ptr - hm.message.ptr);
      } else if (is_chunked && n > 0) {
        walkchunks(c, &hm, (size_t) n);
      }
      // MG_INFO(("---->%d %d\n%.*s", n, is_chunked, (int) c->recv.len,
      // c->recv.buf));
      if (n < 0 && ev == MG_EV_READ) {
        mg_error(c, "HTTP parse:\n%.*s", (int) c->recv.len, c->recv.buf);
        break;
      } else if (n > 0 && (size_t) c->recv.len >= hm.message.len) {
        mg_call(c, MG_EV_HTTP_MSG, &hm);
        mg_iobuf_del(&c->recv, 0, hm.message.len);
      } else {
        if (n > 0 && !is_chunked) {
          hm.chunk =
              mg_str_n((char *) &c->recv.buf[n], c->recv.len - (size_t) n);
          // Store remaining body length in c->pfn_data
          if (c->pfn_data == NULL)
            c->pfn_data = (void *) (hm.message.len - (size_t) n);
          mg_call(c, MG_EV_HTTP_CHUNK, &hm);
          if (c->pfn_data == NULL) {
            hm.chunk.len = 0;                   // Last chunk!
            mg_call(c, MG_EV_HTTP_CHUNK, &hm);  // Lest user know
            memmove(c->recv.buf, c->recv.buf + n, c->recv.len - (size_t) n);
            c->recv.len -= (size_t) n;
          }
        }
        break;
      }
    }
  }
  (void) fnd;
  (void) evd;
}

struct mg_connection *mg_http_connect(struct mg_mgr *mgr, const char *url,
                                      mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = mg_connect(mgr, url, fn, fn_data);
  if (c != NULL) c->pfn = http_cb;
  return c;
}

struct mg_connection *mg_http_listen(struct mg_mgr *mgr, const char *url,
                                     mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = mg_listen(mgr, url, fn, fn_data);
  if (c != NULL) c->pfn = http_cb;
  return c;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/iobuf.c"
#endif



#include <string.h>

// Not using memset for zeroing memory, cause it can be dropped by compiler
// See https://github.com/cesanta/mongoose/pull/1265
static void zeromem(volatile unsigned char *buf, size_t len) {
  if (buf != NULL) {
    while (len--) *buf++ = 0;
  }
}

int mg_iobuf_resize(struct mg_iobuf *io, size_t new_size) {
  int ok = 1;
  if (new_size == 0) {
    zeromem(io->buf, io->size);
    free(io->buf);
    io->buf = NULL;
    io->len = io->size = 0;
  } else if (new_size != io->size) {
    // NOTE(lsm): do not use realloc here. Use calloc/free only, to ease the
    // porting to some obscure platforms like FreeRTOS
    void *p = calloc(1, new_size);
    if (p != NULL) {
      size_t len = new_size < io->len ? new_size : io->len;
      if (len > 0) memmove(p, io->buf, len);
      zeromem(io->buf, io->size);
      free(io->buf);
      io->buf = (unsigned char *) p;
      io->size = new_size;
    } else {
      ok = 0;
      MG_ERROR(("%lld->%lld", (uint64_t) io->size, (uint64_t) new_size));
    }
  }
  return ok;
}

int mg_iobuf_init(struct mg_iobuf *io, size_t size) {
  return mg_iobuf_resize(io, size);
}

size_t mg_iobuf_add(struct mg_iobuf *io, size_t ofs, const void *buf,
                    size_t len, size_t chunk_size) {
  size_t new_size = io->len + len;
  if (new_size > io->size) {
    new_size += chunk_size;             // Make sure that io->size
    new_size -= new_size % chunk_size;  // is aligned by chunk_size boundary
    mg_iobuf_resize(io, new_size);      // Attempt to realloc
    if (new_size != io->size) len = 0;  // Realloc failure, append nothing
  }
  if (ofs < io->len) memmove(io->buf + ofs + len, io->buf + ofs, io->len - ofs);
  if (buf != NULL) memmove(io->buf + ofs, buf, len);
  if (ofs > io->len) io->len += ofs - io->len;
  io->len += len;
  return len;
}

size_t mg_iobuf_del(struct mg_iobuf *io, size_t ofs, size_t len) {
  if (ofs > io->len) ofs = io->len;
  if (ofs + len > io->len) len = io->len - ofs;
  if (io->buf) memmove(io->buf + ofs, io->buf + ofs + len, io->len - ofs - len);
  if (io->buf) zeromem(io->buf + io->len - len, len);
  io->len -= len;
  return len;
}

void mg_iobuf_free(struct mg_iobuf *io) {
  mg_iobuf_resize(io, 0);
}

#ifdef MG_ENABLE_LINES
#line 1 "src/log.c"
#endif



#if MG_ENABLE_LOG
static void mg_log_stdout(const void *buf, size_t len, void *userdata) {
  (void) userdata, (void) buf, (void) len;
#if MG_ENABLE_FILE
  fwrite(buf, 1, len, stdout);
#endif
}

static const char *s_spec = "2";
static void (*s_fn)(const void *, size_t, void *) = mg_log_stdout;
static void *s_fn_param = NULL;

void mg_log_set(const char *spec) {
  MG_DEBUG(("Setting log level to %s", spec));
  s_spec = spec;
}

bool mg_log_prefix(int level, const char *file, int line, const char *fname) {
  // static unsigned long seq;
  int max = MG_LL_INFO;
  struct mg_str k, v, s = mg_str(s_spec);
  const char *p = strrchr(file, '/');

  if (s_fn == NULL) return false;

  if (p == NULL) p = strrchr(file, '\\');
  p = p == NULL ? file : p + 1;

  while (mg_commalist(&s, &k, &v)) {
    if (v.len == 0) max = atoi(k.ptr);
    if (v.len > 0 && strncmp(p, k.ptr, k.len) == 0) max = atoi(v.ptr);
  }

  if (level <= max) {
    char buf[41];
    size_t n = mg_snprintf(buf, sizeof(buf), "%llx %d %s:%d:%s", mg_millis(),
                           level, p, line, fname);
    if (n > sizeof(buf) - 2) n = sizeof(buf) - 2;
    while (n < sizeof(buf)) buf[n++] = ' ';
    buf[sizeof(buf) - 1] = '\0';
    s_fn(buf, n - 1, s_fn_param);
    return true;
  } else {
    return false;
  }
}

void mg_log(const char *fmt, ...) {
  char mem[256], *buf = mem;
  va_list ap;
  size_t len;
  va_start(ap, fmt);
  len = mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  va_end(ap);
  s_fn(buf, len, s_fn_param);
  s_fn("\n", 1, s_fn_param);
  if (buf != mem) free(buf);
}

void mg_log_set_callback(void (*fn)(const void *, size_t, void *), void *fnd) {
  s_fn = fn;
  s_fn_param = fnd;
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/md5.c"
#endif
#include <string.h>


#if defined(MG_ENABLE_MD5) && MG_ENABLE_MD5
#if !defined(BYTE_ORDER) && defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __LITTLE_ENDIAN
#endif /* BIG_ENDIAN */
#endif /* BYTE_ORDER */

static void mg_byte_reverse(unsigned char *buf, unsigned longs) {
/* Forrest: MD5 expect LITTLE_ENDIAN, swap if BIG_ENDIAN */
#if BYTE_ORDER == BIG_ENDIAN
  do {
    uint32_t t = (uint32_t)((unsigned) buf[3] << 8 | buf[2]) << 16 |
                 ((unsigned) buf[1] << 8 | buf[0]);
    *(uint32_t *) buf = t;
    buf += 4;
  } while (--longs);
#else
  (void) buf;
  (void) longs;
#endif
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
  (w += f(x, y, z) + data, w = w << s | w >> (32 - s), w += x)

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void mg_md5_init(mg_md5_ctx *ctx) {
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void mg_md5_transform(uint32_t buf[4], uint32_t const in[16]) {
  uint32_t a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

void mg_md5_update(mg_md5_ctx *ctx, const unsigned char *buf, size_t len) {
  uint32_t t;

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t) ctx->bits[1]++;
  ctx->bits[1] += (uint32_t) len >> 29;

  t = (t >> 3) & 0x3f;

  if (t) {
    unsigned char *p = (unsigned char *) ctx->in + t;

    t = 64 - t;
    if (len < t) {
      memcpy(p, buf, len);
      return;
    }
    memcpy(p, buf, t);
    mg_byte_reverse(ctx->in, 16);
    mg_md5_transform(ctx->buf, (uint32_t *) ctx->in);
    buf += t;
    len -= t;
  }

  while (len >= 64) {
    memcpy(ctx->in, buf, 64);
    mg_byte_reverse(ctx->in, 16);
    mg_md5_transform(ctx->buf, (uint32_t *) ctx->in);
    buf += 64;
    len -= 64;
  }

  memcpy(ctx->in, buf, len);
}

void mg_md5_final(mg_md5_ctx *ctx, unsigned char digest[16]) {
  unsigned count;
  unsigned char *p;
  uint32_t *a;

  count = (ctx->bits[0] >> 3) & 0x3F;

  p = ctx->in + count;
  *p++ = 0x80;
  count = 64 - 1 - count;
  if (count < 8) {
    memset(p, 0, count);
    mg_byte_reverse(ctx->in, 16);
    mg_md5_transform(ctx->buf, (uint32_t *) ctx->in);
    memset(ctx->in, 0, 56);
  } else {
    memset(p, 0, count - 8);
  }
  mg_byte_reverse(ctx->in, 14);

  a = (uint32_t *) ctx->in;
  a[14] = ctx->bits[0];
  a[15] = ctx->bits[1];

  mg_md5_transform(ctx->buf, (uint32_t *) ctx->in);
  mg_byte_reverse((unsigned char *) ctx->buf, 4);
  memcpy(digest, ctx->buf, 16);
  memset((char *) ctx, 0, sizeof(*ctx));
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/mqtt.c"
#endif








#define MQTT_CLEAN_SESSION 0x02
#define MQTT_HAS_WILL 0x04
#define MQTT_WILL_RETAIN 0x20
#define MQTT_HAS_PASSWORD 0x40
#define MQTT_HAS_USER_NAME 0x80

enum { MQTT_OK, MQTT_INCOMPLETE, MQTT_MALFORMED };

void mg_mqtt_send_header(struct mg_connection *c, uint8_t cmd, uint8_t flags,
                         uint32_t len) {
  uint8_t buf[1 + sizeof(len)], *vlen = &buf[1];
  buf[0] = (uint8_t) ((cmd << 4) | flags);
  do {
    *vlen = len % 0x80;
    len /= 0x80;
    if (len > 0) *vlen |= 0x80;
    vlen++;
  } while (len > 0 && vlen < &buf[sizeof(buf)]);
  mg_send(c, buf, (size_t) (vlen - buf));
}

static void mg_send_u16(struct mg_connection *c, uint16_t value) {
  mg_send(c, &value, sizeof(value));
}

void mg_mqtt_login(struct mg_connection *c, struct mg_mqtt_opts *opts) {
  char rnd[9], client_id[16];
  struct mg_str cid = opts->client_id;
  uint32_t total_len = 7 + 1 + 2 + 2;
  uint8_t connflag = (uint8_t) ((opts->will_qos & 3) << 3);

  if (cid.len == 0) {
    mg_random(rnd, sizeof(rnd));
    mg_base64_encode((unsigned char *) rnd, sizeof(rnd), client_id);
    client_id[sizeof(client_id) - 1] = '\0';
    cid = mg_str(client_id);
  }

  if (opts->user.len > 0) {
    total_len += 2 + (uint32_t) opts->user.len;
    connflag |= MQTT_HAS_USER_NAME;
  }
  if (opts->pass.len > 0) {
    total_len += 2 + (uint32_t) opts->pass.len;
    connflag |= MQTT_HAS_PASSWORD;
  }
  if (opts->will_topic.len > 0 && opts->will_message.len > 0) {
    total_len +=
        4 + (uint32_t) opts->will_topic.len + (uint32_t) opts->will_message.len;
    connflag |= MQTT_HAS_WILL;
  }
  if (opts->clean || cid.len == 0) connflag |= MQTT_CLEAN_SESSION;
  if (opts->will_retain) connflag |= MQTT_WILL_RETAIN;
  total_len += (uint32_t) cid.len;

  mg_mqtt_send_header(c, MQTT_CMD_CONNECT, 0, total_len);
  mg_send(c, "\00\04MQTT\04", 7);
  mg_send(c, &connflag, sizeof(connflag));
  // keepalive == 0 means "do not disconnect us!"
  mg_send_u16(c, mg_htons((uint16_t) opts->keepalive));
  mg_send_u16(c, mg_htons((uint16_t) cid.len));
  mg_send(c, cid.ptr, cid.len);
  if (connflag & MQTT_HAS_WILL) {
    mg_send_u16(c, mg_htons((uint16_t) opts->will_topic.len));
    mg_send(c, opts->will_topic.ptr, opts->will_topic.len);
    mg_send_u16(c, mg_htons((uint16_t) opts->will_message.len));
    mg_send(c, opts->will_message.ptr, opts->will_message.len);
  }
  if (opts->user.len > 0) {
    mg_send_u16(c, mg_htons((uint16_t) opts->user.len));
    mg_send(c, opts->user.ptr, opts->user.len);
  }
  if (opts->pass.len > 0) {
    mg_send_u16(c, mg_htons((uint16_t) opts->pass.len));
    mg_send(c, opts->pass.ptr, opts->pass.len);
  }
}

void mg_mqtt_pub(struct mg_connection *c, struct mg_str topic,
                 struct mg_str data, int qos, bool retain) {
  uint8_t flags = (uint8_t) (((qos & 3) << 1) | (retain ? 1 : 0));
  uint32_t total_len = 2 + (uint32_t) topic.len + (uint32_t) data.len;
  MG_DEBUG(("%lu [%.*s] -> [%.*s]", c->id, (int) topic.len, (char *) topic.ptr,
            (int) data.len, (char *) data.ptr));
  if (qos > 0) total_len += 2;
  mg_mqtt_send_header(c, MQTT_CMD_PUBLISH, flags, total_len);
  mg_send_u16(c, mg_htons((uint16_t) topic.len));
  mg_send(c, topic.ptr, topic.len);
  if (qos > 0) {
    static uint16_t s_id;
    if (++s_id == 0) s_id++;
    mg_send_u16(c, mg_htons(s_id));
  }
  mg_send(c, data.ptr, data.len);
}

void mg_mqtt_sub(struct mg_connection *c, struct mg_str topic, int qos) {
  static uint16_t s_id;
  uint8_t qos_ = qos & 3;
  uint32_t total_len = 2 + (uint32_t) topic.len + 2 + 1;
  mg_mqtt_send_header(c, MQTT_CMD_SUBSCRIBE, 2, total_len);
  if (++s_id == 0) ++s_id;
  mg_send_u16(c, mg_htons(s_id));
  mg_send_u16(c, mg_htons((uint16_t) topic.len));
  mg_send(c, topic.ptr, topic.len);
  mg_send(c, &qos_, sizeof(qos_));
}

int mg_mqtt_parse(const uint8_t *buf, size_t len, struct mg_mqtt_message *m) {
  uint8_t lc = 0, *p, *end;
  uint32_t n = 0, len_len = 0;

  memset(m, 0, sizeof(*m));
  m->dgram.ptr = (char *) buf;
  if (len < 2) return MQTT_INCOMPLETE;
  m->cmd = (uint8_t) (buf[0] >> 4);
  m->qos = (buf[0] >> 1) & 3;

  n = len_len = 0;
  p = (uint8_t *) buf + 1;
  while ((size_t) (p - buf) < len) {
    lc = *((uint8_t *) p++);
    n += (uint32_t) ((lc & 0x7f) << 7 * len_len);
    len_len++;
    if (!(lc & 0x80)) break;
    if (len_len >= 4) return MQTT_MALFORMED;
  }
  end = p + n;
  if ((lc & 0x80) || (end > buf + len)) return MQTT_INCOMPLETE;
  m->dgram.len = (size_t) (end - buf);

  switch (m->cmd) {
    case MQTT_CMD_CONNACK:
      if (end - p < 2) return MQTT_MALFORMED;
      m->ack = p[1];
      break;
    case MQTT_CMD_PUBACK:
    case MQTT_CMD_PUBREC:
    case MQTT_CMD_PUBREL:
    case MQTT_CMD_PUBCOMP:
    case MQTT_CMD_SUBACK:
      if (p + 2 > end) return MQTT_MALFORMED;
      m->id = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
      break;
    case MQTT_CMD_SUBSCRIBE: {
      if (p + 2 > end) return MQTT_MALFORMED;
      m->id = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
      p += 2;
      break;
    }
    case MQTT_CMD_PUBLISH: {
      if (p + 2 > end) return MQTT_MALFORMED;
      m->topic.len = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
      m->topic.ptr = (char *) p + 2;
      p += 2 + m->topic.len;
      if (p > end) return MQTT_MALFORMED;
      if (m->qos > 0) {
        if (p + 2 > end) return MQTT_MALFORMED;
        m->id = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
        p += 2;
      }
      if (p > end) return MQTT_MALFORMED;
      m->data.ptr = (char *) p;
      m->data.len = (size_t) (end - p);
      break;
    }
    default:
      break;
  }
  return MQTT_OK;
}

static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos,
                                 size_t pos) {
  unsigned char *buf = (unsigned char *) msg->dgram.ptr + pos;
  size_t new_pos;
  if (pos >= msg->dgram.len) return 0;

  topic->len = (size_t) (((unsigned) buf[0]) << 8 | buf[1]);
  topic->ptr = (char *) buf + 2;
  new_pos = pos + 2 + topic->len + (qos == NULL ? 0 : 1);
  if ((size_t) new_pos > msg->dgram.len) return 0;
  if (qos != NULL) *qos = buf[2 + topic->len];
  return new_pos;
}

size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                        uint8_t *qos, size_t pos) {
  uint8_t tmp;
  return mg_mqtt_next_topic(msg, topic, qos == NULL ? &tmp : qos, pos);
}

size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic,
                          size_t pos) {
  return mg_mqtt_next_topic(msg, topic, NULL, pos);
}

static void mqtt_cb(struct mg_connection *c, int ev, void *ev_data,
                    void *fn_data) {
  if (ev == MG_EV_READ) {
    for (;;) {
      struct mg_mqtt_message mm;
      int rc = mg_mqtt_parse(c->recv.buf, c->recv.len, &mm);
      if (rc == MQTT_MALFORMED) {
        MG_ERROR(("%lu MQTT malformed message", c->id));
        c->is_closing = 1;
        break;
      } else if (rc == MQTT_OK) {
        MG_VERBOSE(("%p MQTT CMD %d len %d [%.*s]", c->fd, mm.cmd,
                    (int) mm.dgram.len, (int) mm.data.len, mm.data.ptr));
        switch (mm.cmd) {
          case MQTT_CMD_CONNACK:
            mg_call(c, MG_EV_MQTT_OPEN, &mm.ack);
            if (mm.ack == 0) {
              MG_DEBUG(("%lu Connected", c->id));
            } else {
              MG_ERROR(("%lu MQTT auth failed, code %d", c->id, mm.ack));
              c->is_closing = 1;
            }
            break;
          case MQTT_CMD_PUBLISH: {
            MG_DEBUG(("%lu [%.*s] -> [%.*s]", c->id, (int) mm.topic.len,
                      mm.topic.ptr, (int) mm.data.len, mm.data.ptr));
            mg_call(c, MG_EV_MQTT_MSG, &mm);
            break;
          }
        }
        mg_call(c, MG_EV_MQTT_CMD, &mm);
        mg_iobuf_del(&c->recv, 0, mm.dgram.len);
      } else {
        break;
      }
    }
  }
  (void) ev_data;
  (void) fn_data;
}

void mg_mqtt_ping(struct mg_connection *nc) {
  mg_mqtt_send_header(nc, MQTT_CMD_PINGREQ, 0, 0);
}

void mg_mqtt_pong(struct mg_connection *nc) {
  mg_mqtt_send_header(nc, MQTT_CMD_PINGRESP, 0, 0);
}

void mg_mqtt_disconnect(struct mg_connection *nc) {
  mg_mqtt_send_header(nc, MQTT_CMD_DISCONNECT, 0, 0);
}

struct mg_connection *mg_mqtt_connect(struct mg_mgr *mgr, const char *url,
                                      struct mg_mqtt_opts *opts,
                                      mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = mg_connect(mgr, url, fn, fn_data);
  if (c != NULL) {
    struct mg_mqtt_opts empty;
    memset(&empty, 0, sizeof(empty));
    mg_mqtt_login(c, opts == NULL ? &empty : opts);
    c->pfn = mqtt_cb;
  }
  return c;
}

struct mg_connection *mg_mqtt_listen(struct mg_mgr *mgr, const char *url,
                                     mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = mg_listen(mgr, url, fn, fn_data);
  if (c != NULL) c->pfn = mqtt_cb, c->pfn_data = mgr;
  return c;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/net.c"
#endif






size_t mg_vprintf(struct mg_connection *c, const char *fmt, va_list ap) {
  char mem[256], *buf = mem;
  size_t len = mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  len = mg_send(c, buf, len);
  if (buf != mem) free(buf);
  return len;
}

size_t mg_printf(struct mg_connection *c, const char *fmt, ...) {
  size_t len = 0;
  va_list ap;
  va_start(ap, fmt);
  len = mg_vprintf(c, fmt, ap);
  va_end(ap);
  return len;
}

char *mg_straddr(struct mg_addr *a, char *buf, size_t len) {
  char tmp[30];
  const char *fmt = a->is_ip6 ? "[%s]:%d" : "%s:%d";
  mg_ntoa(a, tmp, sizeof(tmp));
  mg_snprintf(buf, len, fmt, tmp, (int) mg_ntohs(a->port));
  return buf;
}

char *mg_ntoa(const struct mg_addr *addr, char *buf, size_t len) {
  if (addr->is_ip6) {
    uint16_t *p = (uint16_t *) addr->ip6;
    mg_snprintf(buf, len, "%x:%x:%x:%x:%x:%x:%x:%x", mg_htons(p[0]),
                mg_htons(p[1]), mg_htons(p[2]), mg_htons(p[3]), mg_htons(p[4]),
                mg_htons(p[5]), mg_htons(p[6]), mg_htons(p[7]));
  } else {
    uint8_t p[4];
    memcpy(p, &addr->ip, sizeof(p));
    mg_snprintf(buf, len, "%d.%d.%d.%d", (int) p[0], (int) p[1], (int) p[2],
                (int) p[3]);
  }
  return buf;
}

static bool mg_atonl(struct mg_str str, struct mg_addr *addr) {
  if (mg_vcasecmp(&str, "localhost") != 0) return false;
  addr->ip = mg_htonl(0x7f000001);
  addr->is_ip6 = false;
  return true;
}

static bool mg_atone(struct mg_str str, struct mg_addr *addr) {
  if (str.len > 0) return false;
  addr->ip = 0;
  addr->is_ip6 = false;
  return true;
}

static bool mg_aton4(struct mg_str str, struct mg_addr *addr) {
  uint8_t data[4] = {0, 0, 0, 0};
  size_t i, num_dots = 0;
  for (i = 0; i < str.len; i++) {
    if (str.ptr[i] >= '0' && str.ptr[i] <= '9') {
      int octet = data[num_dots] * 10 + (str.ptr[i] - '0');
      if (octet > 255) return false;
      data[num_dots] = (uint8_t) octet;
    } else if (str.ptr[i] == '.') {
      if (num_dots >= 3 || i == 0 || str.ptr[i - 1] == '.') return false;
      num_dots++;
    } else {
      return false;
    }
  }
  if (num_dots != 3 || str.ptr[i - 1] == '.') return false;
  memcpy(&addr->ip, data, sizeof(data));
  addr->is_ip6 = false;
  return true;
}

static bool mg_v4mapped(struct mg_str str, struct mg_addr *addr) {
  int i;
  if (str.len < 14) return false;
  if (str.ptr[0] != ':' || str.ptr[1] != ':' || str.ptr[6] != ':') return false;
  for (i = 2; i < 6; i++) {
    if (str.ptr[i] != 'f' && str.ptr[i] != 'F') return false;
  }
  if (!mg_aton4(mg_str_n(&str.ptr[7], str.len - 7), addr)) return false;
  memset(addr->ip6, 0, sizeof(addr->ip6));
  addr->ip6[10] = addr->ip6[11] = 255;
  memcpy(&addr->ip6[12], &addr->ip, 4);
  addr->is_ip6 = true;
  return true;
}

static bool mg_aton6(struct mg_str str, struct mg_addr *addr) {
  size_t i, j = 0, n = 0, dc = 42;
  if (str.len > 2 && str.ptr[0] == '[') str.ptr++, str.len -= 2;
  if (mg_v4mapped(str, addr)) return true;
  for (i = 0; i < str.len; i++) {
    if ((str.ptr[i] >= '0' && str.ptr[i] <= '9') ||
        (str.ptr[i] >= 'a' && str.ptr[i] <= 'f') ||
        (str.ptr[i] >= 'A' && str.ptr[i] <= 'F')) {
      unsigned long val;
      if (i > j + 3) return false;
      // MG_DEBUG(("%zu %zu [%.*s]", i, j, (int) (i - j + 1), &str.ptr[j]));
      val = mg_unhexn(&str.ptr[j], i - j + 1);
      addr->ip6[n] = (uint8_t) ((val >> 8) & 255);
      addr->ip6[n + 1] = (uint8_t) (val & 255);
    } else if (str.ptr[i] == ':') {
      j = i + 1;
      if (i > 0 && str.ptr[i - 1] == ':') {
        dc = n;  // Double colon
        if (i > 1 && str.ptr[i - 2] == ':') return false;
      } else if (i > 0) {
        n += 2;
      }
      if (n > 14) return false;
      addr->ip6[n] = addr->ip6[n + 1] = 0;  // For trailing ::
    } else {
      return false;
    }
  }
  if (n < 14 && dc == 42) return false;
  if (n < 14) {
    memmove(&addr->ip6[dc + (14 - n)], &addr->ip6[dc], n - dc + 2);
    memset(&addr->ip6[dc], 0, 14 - n);
  }
  addr->is_ip6 = true;
  return true;
}

bool mg_aton(struct mg_str str, struct mg_addr *addr) {
  // MG_INFO(("[%.*s]", (int) str.len, str.ptr));
  return mg_atone(str, addr) || mg_atonl(str, addr) || mg_aton4(str, addr) ||
         mg_aton6(str, addr);
}

struct mg_connection *mg_alloc_conn(struct mg_mgr *mgr) {
  struct mg_connection *c = (struct mg_connection *) calloc(1, sizeof(*c));
  if (c != NULL) {
    c->mgr = mgr;
    c->id = ++mgr->nextid;
  }
  return c;
}

void mg_close_conn(struct mg_connection *c) {
  mg_resolve_cancel(c);  // Close any pending DNS query
  LIST_DELETE(struct mg_connection, &c->mgr->conns, c);
  if (c == c->mgr->dns4.c) c->mgr->dns4.c = NULL;
  if (c == c->mgr->dns6.c) c->mgr->dns6.c = NULL;
  // Order of operations is important. `MG_EV_CLOSE` event must be fired
  // before we deallocate received data, see #1331
  mg_call(c, MG_EV_CLOSE, NULL);
  MG_DEBUG(("%lu closed", c->id));

  mg_tls_free(c);
  mg_iobuf_free(&c->recv);
  mg_iobuf_free(&c->send);
  memset(c, 0, sizeof(*c));
  free(c);
}

struct mg_connection *mg_connect(struct mg_mgr *mgr, const char *url,
                                 mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = NULL;
  if (url == NULL || url[0] == '\0') {
    MG_ERROR(("null url"));
  } else if ((c = mg_alloc_conn(mgr)) == NULL) {
    MG_ERROR(("OOM"));
  } else {
    LIST_ADD_HEAD(struct mg_connection, &mgr->conns, c);
    c->is_udp = (strncmp(url, "udp:", 4) == 0);
    c->fn = fn;
    c->is_client = true;
    c->fn_data = fn_data;
    MG_DEBUG(("%lu -> %s", c->id, url));
    mg_call(c, MG_EV_OPEN, NULL);
    mg_resolve(c, url);
  }
  return c;
}

struct mg_connection *mg_listen(struct mg_mgr *mgr, const char *url,
                                mg_event_handler_t fn, void *fn_data) {
  struct mg_connection *c = NULL;
  if ((c = mg_alloc_conn(mgr)) == NULL) {
    MG_ERROR(("OOM %s", url));
  } else if (!mg_open_listener(c, url)) {
    MG_ERROR(("Failed: %s, errno %d", url, errno));
    free(c);
  } else {
    c->is_listening = 1;
    c->is_udp = strncmp(url, "udp:", 4) == 0;
    LIST_ADD_HEAD(struct mg_connection, &mgr->conns, c);
    c->fn = fn;
    c->fn_data = fn_data;
    mg_call(c, MG_EV_OPEN, NULL);
    MG_DEBUG(("%lu %s port %u", c->id, url, mg_ntohs(c->rem.port)));
  }
  return c;
}

void mg_mgr_free(struct mg_mgr *mgr) {
  struct mg_connection *c;
  for (c = mgr->conns; c != NULL; c = c->next) c->is_closing = 1;
  mg_mgr_poll(mgr, 0);
#if MG_ARCH == MG_ARCH_FREERTOS_TCP
  FreeRTOS_DeleteSocketSet(mgr->ss);
#endif
  MG_DEBUG(("All connections closed"));
}

void mg_mgr_init(struct mg_mgr *mgr) {
  memset(mgr, 0, sizeof(*mgr));
#if MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
  // clang-format off
  { WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); }
  // clang-format on
#elif MG_ARCH == MG_ARCH_FREERTOS_TCP
  mgr->ss = FreeRTOS_CreateSocketSet();
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
  // Ignore SIGPIPE signal, so if client cancels the request, it
  // won't kill the whole process.
  signal(SIGPIPE, SIG_IGN);
#endif
  mgr->dnstimeout = 3000;
  mgr->dns4.url = "udp://8.8.8.8:53";
  mgr->dns6.url = "udp://[2001:4860:4860::8888]:53";
}

#ifdef MG_ENABLE_LINES
#line 1 "src/sha1.c"
#endif
/* Copyright(c) By Steve Reid <steve@edmweb.com> */
/* 100% Public Domain */

#include <string.h>

/*
 * clang with std=-c99 uses __LITTLE_ENDIAN, by default
 * while for ex, RTOS gcc - LITTLE_ENDIAN, by default
 * it depends on __USE_BSD, but let's have everything
 */
#if !defined(BYTE_ORDER) && defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __LITTLE_ENDIAN
#endif /* BIG_ENDIAN */
#endif /* BYTE_ORDER */

union char64long16 {
  unsigned char c[64];
  uint32_t l[16];
};

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static uint32_t blk0(union char64long16 *block, int i) {
/* Forrest: SHA expect BIG_ENDIAN, swap if LITTLE_ENDIAN */
#if BYTE_ORDER == LITTLE_ENDIAN
  block->l[i] =
      (rol(block->l[i], 24) & 0xFF00FF00) | (rol(block->l[i], 8) & 0x00FF00FF);
#endif
  return block->l[i];
}

/* Avoid redefine warning (ARM /usr/include/sys/ucontext.h define R0~R4) */
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

#define blk(i)                                                               \
  (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ \
                              block->l[(i + 2) & 15] ^ block->l[i & 15],     \
                          1))
#define R0(v, w, x, y, z, i)                                          \
  z += ((w & (x ^ y)) ^ y) + blk0(block, i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                  \
  z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R2(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); \
  w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                        \
  z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
  w = rol(w, 30);
#define R4(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
  w = rol(w, 30);

static void mg_sha1_transform(uint32_t state[5],
                              const unsigned char buffer[64]) {
  uint32_t a, b, c, d, e;
  union char64long16 block[1];

  memcpy(block, buffer, 64);
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  R0(a, b, c, d, e, 0);
  R0(e, a, b, c, d, 1);
  R0(d, e, a, b, c, 2);
  R0(c, d, e, a, b, 3);
  R0(b, c, d, e, a, 4);
  R0(a, b, c, d, e, 5);
  R0(e, a, b, c, d, 6);
  R0(d, e, a, b, c, 7);
  R0(c, d, e, a, b, 8);
  R0(b, c, d, e, a, 9);
  R0(a, b, c, d, e, 10);
  R0(e, a, b, c, d, 11);
  R0(d, e, a, b, c, 12);
  R0(c, d, e, a, b, 13);
  R0(b, c, d, e, a, 14);
  R0(a, b, c, d, e, 15);
  R1(e, a, b, c, d, 16);
  R1(d, e, a, b, c, 17);
  R1(c, d, e, a, b, 18);
  R1(b, c, d, e, a, 19);
  R2(a, b, c, d, e, 20);
  R2(e, a, b, c, d, 21);
  R2(d, e, a, b, c, 22);
  R2(c, d, e, a, b, 23);
  R2(b, c, d, e, a, 24);
  R2(a, b, c, d, e, 25);
  R2(e, a, b, c, d, 26);
  R2(d, e, a, b, c, 27);
  R2(c, d, e, a, b, 28);
  R2(b, c, d, e, a, 29);
  R2(a, b, c, d, e, 30);
  R2(e, a, b, c, d, 31);
  R2(d, e, a, b, c, 32);
  R2(c, d, e, a, b, 33);
  R2(b, c, d, e, a, 34);
  R2(a, b, c, d, e, 35);
  R2(e, a, b, c, d, 36);
  R2(d, e, a, b, c, 37);
  R2(c, d, e, a, b, 38);
  R2(b, c, d, e, a, 39);
  R3(a, b, c, d, e, 40);
  R3(e, a, b, c, d, 41);
  R3(d, e, a, b, c, 42);
  R3(c, d, e, a, b, 43);
  R3(b, c, d, e, a, 44);
  R3(a, b, c, d, e, 45);
  R3(e, a, b, c, d, 46);
  R3(d, e, a, b, c, 47);
  R3(c, d, e, a, b, 48);
  R3(b, c, d, e, a, 49);
  R3(a, b, c, d, e, 50);
  R3(e, a, b, c, d, 51);
  R3(d, e, a, b, c, 52);
  R3(c, d, e, a, b, 53);
  R3(b, c, d, e, a, 54);
  R3(a, b, c, d, e, 55);
  R3(e, a, b, c, d, 56);
  R3(d, e, a, b, c, 57);
  R3(c, d, e, a, b, 58);
  R3(b, c, d, e, a, 59);
  R4(a, b, c, d, e, 60);
  R4(e, a, b, c, d, 61);
  R4(d, e, a, b, c, 62);
  R4(c, d, e, a, b, 63);
  R4(b, c, d, e, a, 64);
  R4(a, b, c, d, e, 65);
  R4(e, a, b, c, d, 66);
  R4(d, e, a, b, c, 67);
  R4(c, d, e, a, b, 68);
  R4(b, c, d, e, a, 69);
  R4(a, b, c, d, e, 70);
  R4(e, a, b, c, d, 71);
  R4(d, e, a, b, c, 72);
  R4(c, d, e, a, b, 73);
  R4(b, c, d, e, a, 74);
  R4(a, b, c, d, e, 75);
  R4(e, a, b, c, d, 76);
  R4(d, e, a, b, c, 77);
  R4(c, d, e, a, b, 78);
  R4(b, c, d, e, a, 79);
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  /* Erase working structures. The order of operations is important,
   * used to ensure that compiler doesn't optimize those out. */
  memset(block, 0, sizeof(block));
  a = b = c = d = e = 0;
  (void) a;
  (void) b;
  (void) c;
  (void) d;
  (void) e;
}

void mg_sha1_init(mg_sha1_ctx *context) {
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[0] = context->count[1] = 0;
}

void mg_sha1_update(mg_sha1_ctx *context, const unsigned char *data,
                    size_t len) {
  size_t i, j;

  j = context->count[0];
  if ((context->count[0] += (uint32_t) len << 3) < j) context->count[1]++;
  context->count[1] += (uint32_t) (len >> 29);
  j = (j >> 3) & 63;
  if ((j + len) > 63) {
    memcpy(&context->buffer[j], data, (i = 64 - j));
    mg_sha1_transform(context->state, context->buffer);
    for (; i + 63 < len; i += 64) {
      mg_sha1_transform(context->state, &data[i]);
    }
    j = 0;
  } else
    i = 0;
  memcpy(&context->buffer[j], &data[i], len - i);
}

void mg_sha1_final(unsigned char digest[20], mg_sha1_ctx *context) {
  unsigned i;
  unsigned char finalcount[8], c;

  for (i = 0; i < 8; i++) {
    finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)] >>
                                      ((3 - (i & 3)) * 8)) &
                                     255);
  }
  c = 0200;
  mg_sha1_update(context, &c, 1);
  while ((context->count[0] & 504) != 448) {
    c = 0000;
    mg_sha1_update(context, &c, 1);
  }
  mg_sha1_update(context, finalcount, 8);
  for (i = 0; i < 20; i++) {
    digest[i] =
        (unsigned char) ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
  memset(context, '\0', sizeof(*context));
  memset(&finalcount, '\0', sizeof(finalcount));
}

#ifdef MG_ENABLE_LINES
#line 1 "src/sntp.c"
#endif






#define SNTP_INTERVAL_SEC 3600
#define SNTP_TIME_OFFSET 2208988800UL

int64_t mg_sntp_parse(const unsigned char *buf, size_t len) {
  int64_t res = -1;
  int mode = len > 0 ? buf[0] & 7 : 0;
  if (len < 48) {
    MG_ERROR(("%s", "corrupt packet"));
  } else if ((buf[0] & 0x38) >> 3 != 4) {
    MG_ERROR(("%s", "wrong version"));
  } else if (mode != 4 && mode != 5) {
    MG_ERROR(("%s", "not a server reply"));
  } else if (buf[1] == 0) {
    MG_ERROR(("%s", "server sent a kiss of death"));
  } else {
    uint32_t *data = (uint32_t *) &buf[40];
    unsigned long seconds = mg_ntohl(data[0]) - SNTP_TIME_OFFSET;
    unsigned long useconds = mg_ntohl(data[1]);
    // MG_DEBUG(("%lu %lu %lu", time(0), seconds, useconds));
    res = ((int64_t) seconds) * 1000 + (int64_t) ((useconds / 1000) % 1000);
  }
  return res;
}

static void sntp_cb(struct mg_connection *c, int ev, void *evd, void *fnd) {
  if (ev == MG_EV_READ) {
    int64_t milliseconds = mg_sntp_parse(c->recv.buf, c->recv.len);
    if (milliseconds > 0) {
      mg_call(c, MG_EV_SNTP_TIME, &milliseconds);
      MG_DEBUG(("%u.%u", (unsigned) (milliseconds / 1000),
                (unsigned) (milliseconds % 1000)));
    }
    c->recv.len = 0;  // Clear receive buffer
  } else if (ev == MG_EV_CONNECT) {
    mg_sntp_send(c, (unsigned long) 0);
  } else if (ev == MG_EV_CLOSE) {
  }
  (void) fnd;
  (void) evd;
}

void mg_sntp_send(struct mg_connection *c, unsigned long utc) {
  if (c->is_resolving) {
    MG_ERROR(("%lu wait until resolved", c->id));
  } else {
    uint8_t buf[48] = {0};
    buf[0] = (0 << 6) | (4 << 3) | 3;
    mg_send(c, buf, sizeof(buf));
    MG_DEBUG(("%lu ct %lu", c->id, utc));
  }
}

struct mg_connection *mg_sntp_connect(struct mg_mgr *mgr, const char *url,
                                      mg_event_handler_t fn, void *fnd) {
  struct mg_connection *c = NULL;
  if (url == NULL) url = "udp://time.google.com:123";
  if ((c = mg_connect(mgr, url, fn, fnd)) != NULL) c->pfn = sntp_cb;
  return c;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/sock.c"
#endif










#if MG_ENABLE_SOCKET
#if MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
#define MG_SOCK_ERRNO WSAGetLastError()
#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int) (~SO_REUSEADDR))
#endif
#elif MG_ARCH == MG_ARCH_FREERTOS_TCP
#define MG_SOCK_ERRNO errno
typedef Socket_t SOCKET;
#define INVALID_SOCKET FREERTOS_INVALID_SOCKET
#else
#define MG_SOCK_ERRNO errno
#ifndef closesocket
#define closesocket(x) close(x)
#endif
#define INVALID_SOCKET (-1)
typedef int SOCKET;
#endif

#define FD(c_) ((SOCKET) (size_t) (c_)->fd)
#define S2PTR(s_) ((void *) (size_t) (s_))

#ifndef MSG_NONBLOCKING
#define MSG_NONBLOCKING 0
#endif

#ifndef AF_INET6
#define AF_INET6 10
#endif

union usa {
  struct sockaddr sa;
  struct sockaddr_in sin;
#if MG_ENABLE_IPV6
  struct sockaddr_in6 sin6;
#endif
};

static socklen_t tousa(struct mg_addr *a, union usa *usa) {
  socklen_t len = sizeof(usa->sin);
  memset(usa, 0, sizeof(*usa));
  usa->sin.sin_family = AF_INET;
  usa->sin.sin_port = a->port;
  *(uint32_t *) &usa->sin.sin_addr = a->ip;
#if MG_ENABLE_IPV6
  if (a->is_ip6) {
    usa->sin.sin_family = AF_INET6;
    usa->sin6.sin6_port = a->port;
    memcpy(&usa->sin6.sin6_addr, a->ip6, sizeof(a->ip6));
    len = sizeof(usa->sin6);
  }
#endif
  return len;
}

static void tomgaddr(union usa *usa, struct mg_addr *a, bool is_ip6) {
  a->is_ip6 = is_ip6;
  a->port = usa->sin.sin_port;
  memcpy(&a->ip, &usa->sin.sin_addr, sizeof(a->ip));
#if MG_ENABLE_IPV6
  if (is_ip6) {
    memcpy(a->ip6, &usa->sin6.sin6_addr, sizeof(a->ip6));
    a->port = usa->sin6.sin6_port;
  }
#endif
}

bool mg_sock_would_block(void);
bool mg_sock_would_block(void) {
  int err = MG_SOCK_ERRNO;
  return err == EINPROGRESS || err == EWOULDBLOCK
#ifndef WINCE
         || err == EAGAIN || err == EINTR
#endif
#if MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
         || err == WSAEINTR || err == WSAEWOULDBLOCK
#endif
      ;
}

static void iolog(struct mg_connection *c, char *buf, long n, bool r) {
  int log_level = n > 0 ? MG_LL_VERBOSE : MG_LL_DEBUG;
  char flags[] = {(char) ('0' + c->is_listening),
                  (char) ('0' + c->is_client),
                  (char) ('0' + c->is_accepted),
                  (char) ('0' + c->is_resolving),
                  (char) ('0' + c->is_connecting),
                  (char) ('0' + c->is_tls),
                  (char) ('0' + c->is_tls_hs),
                  (char) ('0' + c->is_udp),
                  (char) ('0' + c->is_websocket),
                  (char) ('0' + c->is_hexdumping),
                  (char) ('0' + c->is_draining),
                  (char) ('0' + c->is_closing),
                  (char) ('0' + c->is_readable),
                  (char) ('0' + c->is_writable),
                  '\0'};
  MG_LOG(log_level,
         ("%lu %s %d:%d %ld err %d (%s)", c->id, flags, (int) c->send.len,
          (int) c->recv.len, n, MG_SOCK_ERRNO, strerror(errno)));
  if (n == 0) {
    // Do nothing
  } else if (n < 0) {
    c->is_closing = 1;  // Error, or normal termination
  } else if (n > 0) {
    if (c->is_hexdumping) {
      union usa usa;
      char t1[50] = "", t2[50] = "";
      socklen_t slen = sizeof(usa.sin);
      char *s = mg_hexdump(buf, (size_t) n);
      struct mg_addr a;
      memset(&usa, 0, sizeof(usa));
      memset(&a, 0, sizeof(a));
      if (getsockname(FD(c), &usa.sa, &slen) < 0) (void) 0;  // Ignore result
      tomgaddr(&usa, &a, c->rem.is_ip6);
      MG_INFO(("\n-- %lu %s %s %s %s %ld\n%s", c->id,
               mg_straddr(&a, t1, sizeof(t1)), r ? "<-" : "->",
               mg_straddr(&c->rem, t2, sizeof(t2)), c->label, n, s));
      free(s);
      (void) t1, (void) t2;  // Silence warnings for MG_ENABLE_LOG=0
    }
    if (r) {
      struct mg_str evd = mg_str_n(buf, (size_t) n);
      c->recv.len += (size_t) n;
      mg_call(c, MG_EV_READ, &evd);
    } else {
      mg_iobuf_del(&c->send, 0, (size_t) n);
      // if (c->send.len == 0) mg_iobuf_resize(&c->send, 0);
      mg_call(c, MG_EV_WRITE, &n);
    }
  }
}

static long mg_sock_send(struct mg_connection *c, const void *buf, size_t len) {
  long n;
  if (c->is_udp) {
    union usa usa;
    socklen_t slen = tousa(&c->rem, &usa);
    n = sendto(FD(c), (char *) buf, len, 0, &usa.sa, slen);
  } else {
    n = send(FD(c), (char *) buf, len, MSG_NONBLOCKING);
  }
  return n == 0 ? -1 : n < 0 && mg_sock_would_block() ? 0 : n;
}

bool mg_send(struct mg_connection *c, const void *buf, size_t len) {
  if (c->is_udp) {
    long n = mg_sock_send(c, buf, len);
    iolog(c, (char *) buf, n, false);
    return n > 0;
  } else {
    return mg_iobuf_add(&c->send, c->send.len, buf, len, MG_IO_SIZE);
  }
}

static void mg_set_non_blocking_mode(SOCKET fd) {
#if MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
  unsigned long on = 1;
  ioctlsocket(fd, FIONBIO, &on);
#elif MG_ARCH == MG_ARCH_FREERTOS_TCP
  const BaseType_t off = 0;
  if (setsockopt(fd, 0, FREERTOS_SO_RCVTIMEO, &off, sizeof(off)) != 0) (void) 0;
  if (setsockopt(fd, 0, FREERTOS_SO_SNDTIMEO, &off, sizeof(off)) != 0) (void) 0;
#elif MG_ARCH == MG_ARCH_FREERTOS_LWIP || MG_ARCH == MG_ARCH_RTX_LWIP
  lwip_fcntl(fd, F_SETFL, O_NONBLOCK);
#elif MG_ARCH == MG_ARCH_AZURERTOS
  fcntl(fd, F_SETFL, O_NONBLOCK);
#else
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);  // Non-blocking mode
  fcntl(fd, F_SETFD, FD_CLOEXEC);                          // Set close-on-exec
#endif
}

bool mg_open_listener(struct mg_connection *c, const char *url) {
  SOCKET fd = INVALID_SOCKET;
  int s_err = 0;  // Memoized socket error, in case closesocket() overrides it
  c->loc.port = mg_htons(mg_url_port(url));
  if (!mg_aton(mg_url_host(url), &c->loc)) {
    MG_ERROR(("invalid listening URL: %s", url));
  } else {
    union usa usa;
    socklen_t slen = tousa(&c->loc, &usa);
    int on = 1, af = c->loc.is_ip6 ? AF_INET6 : AF_INET;
    int type = strncmp(url, "udp:", 4) == 0 ? SOCK_DGRAM : SOCK_STREAM;
    int proto = type == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;
    (void) on;

    if ((fd = socket(af, type, proto)) != INVALID_SOCKET &&
#if (MG_ARCH != MG_ARCH_WIN32 || !defined(SO_EXCLUSIVEADDRUSE)) && \
    (!defined(LWIP_SOCKET) || (defined(LWIP_SOCKET) && SO_REUSE == 1))
        // 1. SO_RESUSEADDR is not enabled on Windows because the semantics of
        //    SO_REUSEADDR on UNIX and Windows is different. On Windows,
        //    SO_REUSEADDR allows to bind a socket to a port without error even
        //    if the port is already open by another program. This is not the
        //    behavior SO_REUSEADDR was designed for, and leads to hard-to-track
        //    failure scenarios. Therefore, SO_REUSEADDR was disabled on Windows
        //    unless SO_EXCLUSIVEADDRUSE is supported and set on a socket.
        // 2. In case of LWIP, SO_REUSEADDR should be explicitly enabled, by
        // defining
        //    SO_REUSE (in lwipopts.h), otherwise the code below will compile
        //    but won't work! (setsockopt will return EINVAL)
        !setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) &&
#endif
#if MG_ARCH == MG_ARCH_WIN32 && defined(SO_EXCLUSIVEADDRUSE) && !defined(WINCE)
        // "Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE"
        //! setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &on, sizeof(on))
        //! &&
        !setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &on,
                    sizeof(on)) &&
#endif
        bind(fd, &usa.sa, slen) == 0 &&
        // NOTE(lsm): FreeRTOS uses backlog value as a connection limit
        (type == SOCK_DGRAM || listen(fd, MG_SOCK_LISTEN_BACKLOG_SIZE) == 0)) {
      // In case port was set to 0, get the real port number
      if (getsockname(fd, &usa.sa, &slen) == 0) {
        c->loc.port = usa.sin.sin_port;
#if MG_ENABLE_IPV6
        if (c->loc.is_ip6) c->loc.port = usa.sin6.sin6_port;
#endif
      }
      mg_set_non_blocking_mode(fd);
    } else if (fd != INVALID_SOCKET) {
      s_err = MG_SOCK_ERRNO;
      closesocket(fd);
      fd = INVALID_SOCKET;
    }
  }
  c->fd = S2PTR(fd);
  if (fd == INVALID_SOCKET) {
    if (s_err == 0) s_err = MG_SOCK_ERRNO;
    MG_ERROR(("failed %s, errno %d (%s)", url, s_err, strerror(s_err)));
  }
  return fd != INVALID_SOCKET;
}

static long mg_sock_recv(struct mg_connection *c, void *buf, size_t len) {
  long n = 0;
  if (c->is_udp) {
    union usa usa;
    socklen_t slen = tousa(&c->rem, &usa);
    n = recvfrom(FD(c), (char *) buf, len, 0, &usa.sa, &slen);
    if (n > 0) tomgaddr(&usa, &c->rem, slen != sizeof(usa.sin));
  } else {
    n = recv(FD(c), (char *) buf, len, MSG_NONBLOCKING);
  }
  return n == 0 ? -1 : n < 0 && mg_sock_would_block() ? 0 : n;
}

// NOTE(lsm): do only one iteration of reads, cause some systems
// (e.g. FreeRTOS stack) return 0 instead of -1/EWOULDBLOCK when no data
static long read_conn(struct mg_connection *c) {
  long n = -1;
  if (c->recv.len >= MG_MAX_RECV_BUF_SIZE) {
    mg_error(c, "max_recv_buf_size reached");
  } else if (c->recv.size - c->recv.len < MG_IO_SIZE &&
             !mg_iobuf_resize(&c->recv, c->recv.size + MG_IO_SIZE)) {
    mg_error(c, "oom");
  } else {
    char *buf = (char *) &c->recv.buf[c->recv.len];
    size_t len = c->recv.size - c->recv.len;
    n = c->is_tls ? mg_tls_recv(c, buf, len) : mg_sock_recv(c, buf, len);
    iolog(c, buf, n, true);
  }
  return n;
}

static void write_conn(struct mg_connection *c) {
  char *buf = (char *) c->send.buf;
  size_t len = c->send.len;
  long n = c->is_tls ? mg_tls_send(c, buf, len) : mg_sock_send(c, buf, len);
  iolog(c, buf, n, false);
}

static void close_conn(struct mg_connection *c) {
  if (FD(c) != INVALID_SOCKET) {
    closesocket(FD(c));
#if MG_ARCH == MG_ARCH_FREERTOS_TCP
    FreeRTOS_FD_CLR(c->fd, c->mgr->ss, eSELECT_ALL);
#endif
    c->fd = NULL;
  }
  mg_close_conn(c);
}

static void setsockopts(struct mg_connection *c) {
#if MG_ARCH == MG_ARCH_FREERTOS_TCP || MG_ARCH == MG_ARCH_AZURERTOS
  (void) c;
#else
  int on = 1;
#if !defined(SOL_TCP)
#define SOL_TCP IPPROTO_TCP
#endif
  if (setsockopt(FD(c), SOL_TCP, TCP_NODELAY, (char *) &on, sizeof(on)) != 0)
    (void) 0;
#if defined(TCP_QUICKACK)
  if (setsockopt(FD(c), SOL_TCP, TCP_QUICKACK, (char *) &on, sizeof(on)) != 0)
    (void) 0;
#endif
  if (setsockopt(FD(c), SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on)) !=
      0)
    (void) 0;
#if (defined(ESP32) && ESP32) || (defined(ESP8266) && ESP8266) || \
    defined(__linux__)
  int idle = 60;
  if (setsockopt(FD(c), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) != 0)
    (void) 0;
#endif
#if MG_ARCH != MG_ARCH_WIN32 && !defined(__QNX__) && MG_ARCH != MG_ARCH_ZEPHYR
  {
    int cnt = 3, intvl = 20;
    if (setsockopt(FD(c), IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt)) != 0)
      (void) 0;
    if (setsockopt(FD(c), IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) !=
        0)
      (void) 0;
  }
#endif
#endif
}

void mg_connect_resolved(struct mg_connection *c) {
  // char buf[40];
  int type = c->is_udp ? SOCK_DGRAM : SOCK_STREAM;
  int rc, af = c->rem.is_ip6 ? AF_INET6 : AF_INET;
  // mg_straddr(&c->rem, buf, sizeof(buf));
  c->fd = S2PTR(socket(af, type, 0));
  c->is_resolving = 0;
  if (FD(c) == INVALID_SOCKET) {
    mg_error(c, "socket(): %d", MG_SOCK_ERRNO);
  } else if (c->is_udp) {
    mg_call(c, MG_EV_RESOLVE, NULL);
    mg_call(c, MG_EV_CONNECT, NULL);
  } else {
    union usa usa;
    socklen_t slen = tousa(&c->rem, &usa);
    mg_set_non_blocking_mode(FD(c));
    setsockopts(c);
    mg_call(c, MG_EV_RESOLVE, NULL);
    if ((rc = connect(FD(c), &usa.sa, slen)) == 0) {
      mg_call(c, MG_EV_CONNECT, NULL);
    } else if (mg_sock_would_block()) {
      c->is_connecting = 1;
    } else {
      mg_error(c, "connect: %d", MG_SOCK_ERRNO);
    }
  }
}

static void accept_conn(struct mg_mgr *mgr, struct mg_connection *lsn) {
  struct mg_connection *c = NULL;
  union usa usa;
  socklen_t sa_len = sizeof(usa);
  SOCKET fd = accept(FD(lsn), &usa.sa, &sa_len);
  if (fd == INVALID_SOCKET) {
#if MG_ARCH == MG_ARCH_AZURERTOS
    // AzureRTOS, in non-block socket mode can mark listening socket readable
    // even it is not. See comment for 'select' func implementation in nx_bsd.c
    // That's not an error, just should try later
    if (MG_SOCK_ERRNO != EAGAIN)
#endif
      MG_ERROR(("%lu accept failed, errno %d", lsn->id, MG_SOCK_ERRNO));
#if ((MG_ARCH != MG_ARCH_WIN32) && (MG_ARCH != MG_ARCH_FREERTOS_TCP))
  } else if ((long) fd >= FD_SETSIZE) {
    MG_ERROR(("%ld > %ld", (long) fd, (long) FD_SETSIZE));
    closesocket(fd);
#endif
  } else if ((c = mg_alloc_conn(mgr)) == NULL) {
    MG_ERROR(("%lu OOM", lsn->id));
    closesocket(fd);
  } else {
    char buf[40];
    tomgaddr(&usa, &c->rem, sa_len != sizeof(usa.sin));
    mg_straddr(&c->rem, buf, sizeof(buf));
    MG_DEBUG(("%lu accepted %s", c->id, buf));
    LIST_ADD_HEAD(struct mg_connection, &mgr->conns, c);
    c->fd = S2PTR(fd);
    mg_set_non_blocking_mode(FD(c));
    setsockopts(c);
    c->is_accepted = 1;
    c->is_hexdumping = lsn->is_hexdumping;
    c->pfn = lsn->pfn;
    c->loc = lsn->loc;
    c->pfn_data = lsn->pfn_data;
    c->fn = lsn->fn;
    c->fn_data = lsn->fn_data;
    mg_call(c, MG_EV_OPEN, NULL);
    mg_call(c, MG_EV_ACCEPT, NULL);
  }
}

static bool mg_socketpair(SOCKET sp[2], union usa usa[2]) {
  socklen_t n = sizeof(usa[0].sin);
  bool result = false;

  (void) memset(&usa[0], 0, sizeof(usa[0]));
  usa[0].sin.sin_family = AF_INET;
  *(uint32_t *) &usa->sin.sin_addr = mg_htonl(0x7f000001);  // 127.0.0.1
  usa[1] = usa[0];

  if ((sp[0] = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET &&
      (sp[1] = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET &&
      bind(sp[0], &usa[0].sa, n) == 0 && bind(sp[1], &usa[1].sa, n) == 0 &&
      getsockname(sp[0], &usa[0].sa, &n) == 0 &&
      getsockname(sp[1], &usa[1].sa, &n) == 0 &&
      connect(sp[0], &usa[1].sa, n) == 0 &&
      connect(sp[1], &usa[0].sa, n) == 0) {
    mg_set_non_blocking_mode(sp[1]);  // Set close-on-exec
    result = true;
  } else {
    if (sp[0] != INVALID_SOCKET) closesocket(sp[0]);
    if (sp[1] != INVALID_SOCKET) closesocket(sp[1]);
    sp[0] = sp[1] = INVALID_SOCKET;
  }

  return result;
}

void mg_mgr_wakeup(struct mg_connection *c, const void *buf, size_t len) {
  if (buf == NULL || len == 0) buf = (void *) "", len = 1;
  if ((size_t) send((SOCKET) (size_t) c->pfn_data, (const char *) buf, len,
                    MSG_NONBLOCKING) != len)
    (void) 0;
}

static void pf1(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_READ) {
    mg_iobuf_free(&c->recv);
  } else if (ev == MG_EV_CLOSE) {
    closesocket((SOCKET) (size_t) c->pfn_data);
  }
  (void) ev_data, (void) fn_data;
}

struct mg_connection *mg_mkpipe(struct mg_mgr *mgr, mg_event_handler_t fn,
                                void *fn_data) {
  union usa usa[2];
  SOCKET sp[2] = {INVALID_SOCKET, INVALID_SOCKET};
  struct mg_connection *c = NULL;
  if (!mg_socketpair(sp, usa)) {
    MG_ERROR(("Cannot create socket pair"));
  } else if ((c = mg_alloc_conn(mgr)) == NULL) {
    closesocket(sp[0]);
    closesocket(sp[1]);
    MG_ERROR(("OOM"));
  } else {
    MG_DEBUG(("pipe %lu", (unsigned long) sp[0]));
    tomgaddr(&usa[0], &c->rem, false);
    c->fd = S2PTR(sp[1]);
    c->is_udp = 1;
    c->pfn = pf1;
    c->pfn_data = (void *) (size_t) sp[0];
    c->fn = fn;
    c->fn_data = fn_data;
    mg_call(c, MG_EV_OPEN, NULL);
    LIST_ADD_HEAD(struct mg_connection, &mgr->conns, c);
  }
  return c;
}

static void mg_iotest(struct mg_mgr *mgr, int ms) {
#if MG_ARCH == MG_ARCH_FREERTOS_TCP
  struct mg_connection *c;
  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_closing || c->is_resolving || FD(c) == INVALID_SOCKET) continue;
    FreeRTOS_FD_SET(c->fd, mgr->ss, eSELECT_READ | eSELECT_EXCEPT);
    if (c->is_connecting || (c->send.len > 0 && c->is_tls_hs == 0))
      FreeRTOS_FD_SET(c->fd, mgr->ss, eSELECT_WRITE);
  }
  FreeRTOS_select(mgr->ss, pdMS_TO_TICKS(ms));
  for (c = mgr->conns; c != NULL; c = c->next) {
    EventBits_t bits = FreeRTOS_FD_ISSET(c->fd, mgr->ss);
    c->is_readable = bits & (eSELECT_READ | eSELECT_EXCEPT) ? 1 : 0;
    c->is_writable = bits & eSELECT_WRITE ? 1 : 0;
    FreeRTOS_FD_CLR(c->fd, mgr->ss,
                    eSELECT_READ | eSELECT_EXCEPT | eSELECT_WRITE);
  }
#else
  struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
  struct mg_connection *c;
  fd_set rset, wset;
  SOCKET maxfd = 0;
  int rc;

  FD_ZERO(&rset);
  FD_ZERO(&wset);

  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_closing || c->is_resolving || FD(c) == INVALID_SOCKET) continue;
    FD_SET(FD(c), &rset);
    if (FD(c) > maxfd) maxfd = FD(c);
    if (c->is_connecting || (c->send.len > 0 && c->is_tls_hs == 0))
      FD_SET(FD(c), &wset);
  }

  if ((rc = select((int) maxfd + 1, &rset, &wset, NULL, &tv)) < 0) {
    MG_DEBUG(("select: %d %d", rc, MG_SOCK_ERRNO));
    FD_ZERO(&rset);
    FD_ZERO(&wset);
  }

  for (c = mgr->conns; c != NULL; c = c->next) {
    // TLS might have stuff buffered, so dig everything
    c->is_readable = FD(c) != INVALID_SOCKET && FD_ISSET(FD(c), &rset);
    c->is_writable = FD(c) != INVALID_SOCKET && FD_ISSET(FD(c), &wset);
  }
#endif
}

static void connect_conn(struct mg_connection *c) {
  int rc = 0;
#if MG_ARCH != MG_ARCH_FREERTOS_TCP
  socklen_t len = sizeof(rc);
  if (getsockopt(FD(c), SOL_SOCKET, SO_ERROR, (char *) &rc, &len)) rc = 1;
#endif
  if (rc == EAGAIN || rc == EWOULDBLOCK) rc = 0;
  c->is_connecting = 0;
  if (rc) {
    char buf[50];
    mg_error(c, "error connecting to %s",
             mg_straddr(&c->rem, buf, sizeof(buf)));
  } else {
    if (c->is_tls_hs) mg_tls_handshake(c);
    mg_call(c, MG_EV_CONNECT, NULL);
  }
}

void mg_mgr_poll(struct mg_mgr *mgr, int ms) {
  struct mg_connection *c, *tmp;
  int64_t now;

  mg_iotest(mgr, ms);
  now = mg_millis();
  mg_timer_poll(now);

  for (c = mgr->conns; c != NULL; c = tmp) {
    tmp = c->next;
    mg_call(c, MG_EV_POLL, &now);
    MG_VERBOSE(("%lu %c%c %c%c%c%c%c", c->id, c->is_readable ? 'r' : '-',
                c->is_writable ? 'w' : '-', c->is_tls ? 'T' : 't',
                c->is_connecting ? 'C' : 'c', c->is_tls_hs ? 'H' : 'h',
                c->is_resolving ? 'R' : 'r', c->is_closing ? 'C' : 'c'));
    if (c->is_resolving || c->is_closing) {
      // Do nothing
    } else if (c->is_listening && c->is_udp == 0) {
      if (c->is_readable) accept_conn(mgr, c);
    } else if (c->is_connecting) {
      if (c->is_readable || c->is_writable) connect_conn(c);
    } else if (c->is_tls_hs) {
      if ((c->is_readable || c->is_writable)) mg_tls_handshake(c);
    } else {
      if (c->is_readable) read_conn(c);
      if (c->is_writable) write_conn(c);
    }

    if (c->is_draining && c->send.len == 0) c->is_closing = 1;
    if (c->is_closing) close_conn(c);
  }
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/ssi.c"
#endif




#ifndef MG_MAX_SSI_DEPTH
#define MG_MAX_SSI_DEPTH 5
#endif

#ifndef MG_SSI_BUFSIZ
#define MG_SSI_BUFSIZ 1024
#endif

#if MG_ENABLE_SSI
static char *mg_ssi(const char *path, const char *root, int depth) {
  struct mg_iobuf b = {NULL, 0, 0};
  FILE *fp = fopen(path, "rb");
  if (fp != NULL) {
    char buf[MG_SSI_BUFSIZ] = "", arg[sizeof(buf)] = "";
    int ch, intag = 0;
    size_t len = 0, align = MG_IO_SIZE;
    while ((ch = fgetc(fp)) != EOF) {
      if (intag && ch == '>' && buf[len - 1] == '-' && buf[len - 2] == '-') {
        buf[len++] = (char) (ch & 0xff);
        if (sscanf(buf, "<!--#include file=\"%[^\"]", arg)) {
          char tmp[MG_PATH_MAX + MG_SSI_BUFSIZ + 10],
              *p = (char *) path + strlen(path), *data;
          while (p > path && p[-1] != MG_DIRSEP && p[-1] != '/') p--;
          mg_snprintf(tmp, sizeof(tmp), "%.*s%s", (int) (p - path), path, arg);
          if (depth < MG_MAX_SSI_DEPTH &&
              (data = mg_ssi(tmp, root, depth + 1)) != NULL) {
            mg_iobuf_add(&b, b.len, data, strlen(data), align);
            free(data);
          } else {
            MG_ERROR(("%s: file=%s error or too deep", path, arg));
          }
        } else if (sscanf(buf, "<!--#include virtual=\"%[^\"]", arg)) {
          char tmp[MG_PATH_MAX + MG_SSI_BUFSIZ + 10], *data;
          mg_snprintf(tmp, sizeof(tmp), "%s%s", root, arg);
          if (depth < MG_MAX_SSI_DEPTH &&
              (data = mg_ssi(tmp, root, depth + 1)) != NULL) {
            mg_iobuf_add(&b, b.len, data, strlen(data), align);
            free(data);
          } else {
            MG_ERROR(("%s: virtual=%s error or too deep", path, arg));
          }
        } else {
          // Unknown SSI tag
          MG_ERROR(("Unknown SSI tag: %.*s", (int) len, buf));
          mg_iobuf_add(&b, b.len, buf, len, align);
        }
        intag = 0;
        len = 0;
      } else if (ch == '<') {
        intag = 1;
        if (len > 0) mg_iobuf_add(&b, b.len, buf, len, align);
        len = 0;
        buf[len++] = (char) (ch & 0xff);
      } else if (intag) {
        if (len == 5 && strncmp(buf, "<!--#", 5) != 0) {
          intag = 0;
        } else if (len >= sizeof(buf) - 2) {
          MG_ERROR(("%s: SSI tag is too large", path));
          len = 0;
        }
        buf[len++] = (char) (ch & 0xff);
      } else {
        buf[len++] = (char) (ch & 0xff);
        if (len >= sizeof(buf)) {
          mg_iobuf_add(&b, b.len, buf, len, align);
          len = 0;
        }
      }
    }
    if (len > 0) mg_iobuf_add(&b, b.len, buf, len, align);
    if (b.len > 0) mg_iobuf_add(&b, b.len, "", 1, align);  // nul-terminate
    fclose(fp);
  }
  (void) depth;
  (void) root;
  return (char *) b.buf;
}

void mg_http_serve_ssi(struct mg_connection *c, const char *root,
                       const char *fullpath) {
  const char *headers = "Content-Type: text/html; charset=utf-8\r\n";
  char *data = mg_ssi(fullpath, root, 0);
  mg_http_reply(c, 200, headers, "%s", data == NULL ? "" : data);
  free(data);
}
#else
void mg_http_serve_ssi(struct mg_connection *c, const char *root,
                       const char *fullpath) {
  mg_http_reply(c, 501, NULL, "SSI not enabled");
  (void) root, (void) fullpath;
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/str.c"
#endif

#include <stdlib.h>

struct mg_str mg_str_s(const char *s) {
  struct mg_str str = {s, s == NULL ? 0 : strlen(s)};
  return str;
}

struct mg_str mg_str_n(const char *s, size_t n) {
  struct mg_str str = {s, n};
  return str;
}

int mg_lower(const char *s) {
  int c = *s;
  if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return c;
}

int mg_ncasecmp(const char *s1, const char *s2, size_t len) {
  int diff = 0;
  if (len > 0) do {
      diff = mg_lower(s1++) - mg_lower(s2++);
    } while (diff == 0 && s1[-1] != '\0' && --len > 0);
  return diff;
}

int mg_casecmp(const char *s1, const char *s2) {
  return mg_ncasecmp(s1, s2, (size_t) ~0);
}

int mg_vcmp(const struct mg_str *s1, const char *s2) {
  size_t n2 = strlen(s2), n1 = s1->len;
  int r = strncmp(s1->ptr, s2, (n1 < n2) ? n1 : n2);
  if (r == 0) return (int) (n1 - n2);
  return r;
}

int mg_vcasecmp(const struct mg_str *str1, const char *str2) {
  size_t n2 = strlen(str2), n1 = str1->len;
  int r = mg_ncasecmp(str1->ptr, str2, (n1 < n2) ? n1 : n2);
  if (r == 0) return (int) (n1 - n2);
  return r;
}

struct mg_str mg_strdup(const struct mg_str s) {
  struct mg_str r = {NULL, 0};
  if (s.len > 0 && s.ptr != NULL) {
    char *sc = (char *) calloc(1, s.len + 1);
    if (sc != NULL) {
      memcpy(sc, s.ptr, s.len);
      sc[s.len] = '\0';
      r.ptr = sc;
      r.len = s.len;
    }
  }
  return r;
}

int mg_strcmp(const struct mg_str str1, const struct mg_str str2) {
  size_t i = 0;
  while (i < str1.len && i < str2.len) {
    int c1 = str1.ptr[i];
    int c2 = str2.ptr[i];
    if (c1 < c2) return -1;
    if (c1 > c2) return 1;
    i++;
  }
  if (i < str1.len) return 1;
  if (i < str2.len) return -1;
  return 0;
}

const char *mg_strstr(const struct mg_str haystack,
                      const struct mg_str needle) {
  size_t i;
  if (needle.len > haystack.len) return NULL;
  for (i = 0; i <= haystack.len - needle.len; i++) {
    if (memcmp(haystack.ptr + i, needle.ptr, needle.len) == 0) {
      return haystack.ptr + i;
    }
  }
  return NULL;
}

static bool is_digit(int c) {
  return c >= '0' && c <= '9';
}

static bool is_space(int c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

struct mg_str mg_strstrip(struct mg_str s) {
  while (s.len > 0 && is_space((int) *s.ptr)) s.ptr++, s.len--;
  while (s.len > 0 && is_space((int) *(s.ptr + s.len - 1))) s.len--;
  return s;
}

bool mg_match(struct mg_str s, struct mg_str p, struct mg_str *caps) {
  size_t i = 0, j = 0, ni = 0, nj = 0;
  if (caps) caps->ptr = NULL, caps->len = 0;
  while (i < p.len || j < s.len) {
    if (i < p.len && j < s.len && (p.ptr[i] == '?' || s.ptr[j] == p.ptr[i])) {
      if (caps == NULL) {
      } else if (p.ptr[i] == '?') {
        caps->ptr = &s.ptr[j], caps->len = 1;     // Finalize `?` cap
        caps++, caps->ptr = NULL, caps->len = 0;  // Init next cap
      } else if (caps->ptr != NULL && caps->len == 0) {
        caps->len = (size_t) (&s.ptr[j] - caps->ptr);  // Finalize current cap
        caps++, caps->len = 0, caps->ptr = NULL;       // Init next cap
      }
      i++, j++;
    } else if (i < p.len && (p.ptr[i] == '*' || p.ptr[i] == '#')) {
      if (caps && !caps->ptr) caps->len = 0, caps->ptr = &s.ptr[j];  // Init cap
      ni = i++, nj = j + 1;
    } else if (nj > 0 && nj <= s.len && (p.ptr[ni] == '#' || s.ptr[j] != '/')) {
      i = ni, j = nj;
      if (caps && caps->ptr == NULL && caps->len == 0) {
        caps--, caps->len = 0;  // Restart previous cap
      }
    } else {
      return false;
    }
  }
  if (caps && caps->ptr && caps->len == 0) {
    caps->len = (size_t) (&s.ptr[j] - caps->ptr);
  }
  return true;
}

bool mg_globmatch(const char *s1, size_t n1, const char *s2, size_t n2) {
  return mg_match(mg_str_n(s2, n2), mg_str_n(s1, n1), NULL);
}

static size_t mg_nce(const char *s, size_t n, size_t ofs, size_t *koff,
                     size_t *klen, size_t *voff, size_t *vlen) {
  size_t kvlen, kl;
  for (kvlen = 0; ofs + kvlen < n && s[ofs + kvlen] != ',';) kvlen++;
  for (kl = 0; kl < kvlen && s[ofs + kl] != '=';) kl++;
  if (koff != NULL) *koff = ofs;
  if (klen != NULL) *klen = kl;
  if (voff != NULL) *voff = kl < kvlen ? ofs + kl + 1 : 0;
  if (vlen != NULL) *vlen = kl < kvlen ? kvlen - kl - 1 : 0;
  ofs += kvlen + 1;
  return ofs > n ? n : ofs;
}

bool mg_commalist(struct mg_str *s, struct mg_str *k, struct mg_str *v) {
  size_t koff = 0, klen = 0, voff = 0, vlen = 0, off = 0;
  if (s->ptr == NULL || s->len == 0) return 0;
  off = mg_nce(s->ptr, s->len, 0, &koff, &klen, &voff, &vlen);
  if (k != NULL) *k = mg_str_n(s->ptr + koff, klen);
  if (v != NULL) *v = mg_str_n(s->ptr + voff, vlen);
  *s = mg_str_n(s->ptr + off, s->len - off);
  return off > 0;
}

size_t mg_snprintf(char *buf, size_t len, const char *fmt, ...) {
  va_list ap;
  size_t n;
  va_start(ap, fmt);
  n = mg_vsnprintf(buf, len, fmt, ap);
  va_end(ap);
  return n;
}

char *mg_hexdump(const void *buf, size_t len) {
  const unsigned char *p = (const unsigned char *) buf;
  size_t i, idx, n = 0, ofs = 0, dlen = len * 5 + 100;
  char ascii[17] = "", *dst = (char *) calloc(1, dlen);
  if (dst == NULL) return dst;
  for (i = 0; i < len; i++) {
    idx = i % 16;
    if (idx == 0) {
      if (i > 0 && dlen > n)
        n += mg_snprintf(dst + n, dlen - n, "  %s\n", ascii);
      if (dlen > n)
        n += mg_snprintf(dst + n, dlen - n, "%04x ", (int) (i + ofs));
    }
    if (dlen < n) break;
    n += mg_snprintf(dst + n, dlen - n, " %02x", p[i]);
    ascii[idx] = (char) (p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i]);
    ascii[idx + 1] = '\0';
  }
  while (i++ % 16) {
    if (n < dlen) n += mg_snprintf(dst + n, dlen - n, "%s", "   ");
  }
  if (n < dlen) n += mg_snprintf(dst + n, dlen - n, "  %s\n", ascii);
  if (n > dlen - 1) n = dlen - 1;
  dst[n] = '\0';
  return dst;
}

char *mg_hex(const void *buf, size_t len, char *to) {
  const unsigned char *p = (const unsigned char *) buf;
  static const char *hex = "0123456789abcdef";
  size_t i = 0;
  for (; len--; p++) {
    to[i++] = hex[p[0] >> 4];
    to[i++] = hex[p[0] & 0x0f];
  }
  to[i] = '\0';
  return to;
}

static unsigned char mg_unhex_nimble(unsigned char c) {
  return (c >= '0' && c <= '9')   ? (unsigned char) (c - '0')
         : (c >= 'A' && c <= 'F') ? (unsigned char) (c - '7')
                                  : (unsigned char) (c - 'W');
}

unsigned long mg_unhexn(const char *s, size_t len) {
  unsigned long i = 0, v = 0;
  for (i = 0; i < len; i++) v <<= 4, v |= mg_unhex_nimble(((uint8_t *) s)[i]);
  return v;
}

void mg_unhex(const char *buf, size_t len, unsigned char *to) {
  size_t i;
  for (i = 0; i < len; i += 2) {
    to[i >> 1] = (unsigned char) mg_unhexn(&buf[i], 2);
  }
}

size_t mg_vasprintf(char **buf, size_t size, const char *fmt, va_list ap) {
  va_list ap_copy;
  size_t len;

  va_copy(ap_copy, ap);
  len = mg_vsnprintf(*buf, size, fmt, ap_copy);
  va_end(ap_copy);

  if (len >= size) {
    //  Allocate a buffer that is large enough
    if ((*buf = (char *) calloc(1, len + 1)) == NULL) {
      len = 0;
    } else {
      va_copy(ap_copy, ap);
      len = mg_vsnprintf(*buf, len + 1, fmt, ap_copy);
      va_end(ap_copy);
    }
  }

  return len;
}

size_t mg_asprintf(char **buf, size_t size, const char *fmt, ...) {
  size_t ret;
  va_list ap;
  va_start(ap, fmt);
  ret = mg_vasprintf(buf, size, fmt, ap);
  va_end(ap);
  return ret;
}

int64_t mg_to64(struct mg_str str) {
  int64_t result = 0, neg = 1, max = 922337203685477570 /* INT64_MAX/10-10 */;
  size_t i = 0;
  while (i < str.len && (str.ptr[i] == ' ' || str.ptr[i] == '\t')) i++;
  if (i < str.len && str.ptr[i] == '-') neg = -1, i++;
  while (i < str.len && str.ptr[i] >= '0' && str.ptr[i] <= '9') {
    if (result > max) return 0;
    result *= 10;
    result += (str.ptr[i] - '0');
    i++;
  }
  return result * neg;
}

size_t mg_lld(char *buf, int64_t val, bool is_signed, bool is_hex) {
  const char *letters = "0123456789abcdef";
  uint64_t v = (uint64_t) val;
  size_t s = 0, n, i;
  if (is_signed && val < 0) buf[s++] = '-', v = (uint64_t) (-val);
  // This loop prints a number in reverse order. I guess this is because we
  // write numbers from right to left: least significant digit comes last.
  // Maybe because we use Arabic numbers, and Arabs write RTL?
  if (is_hex) {
    for (n = 0; v; v >>= 4) buf[s + n++] = letters[v & 15];
  } else {
    for (n = 0; v; v /= 10) buf[s + n++] = letters[v % 10];
  }
  // Reverse a string
  for (i = 0; i < n / 2; i++) {
    char t = buf[s + i];
    buf[s + i] = buf[s + n - i - 1], buf[s + n - i - 1] = t;
  }
  if (val == 0) buf[n++] = '0';  // Handle special case
  return n + s;
}

static size_t mg_copys(char *buf, size_t len, size_t n, char *p, size_t k) {
  size_t j = 0;
  for (j = 0; j < k && p[j]; j++)
    if (j + n < len) buf[n + j] = p[j];
  return j;
}

size_t mg_vsnprintf(char *buf, size_t len, const char *fmt, va_list ap) {
  size_t i = 0, n = 0;
  while (fmt[i] != '\0') {
    if (fmt[i] == '%') {
      size_t j, k, x = 0, is_long = 0, w = 0 /* width */, pr = 0 /* prec */;
      char pad = ' ', minus = 0, c = fmt[++i];
      if (c == '#') x++, c = fmt[++i];
      if (c == '-') minus++, c = fmt[++i];
      if (c == '0') pad = '0', c = fmt[++i];
      while (is_digit(c)) w *= 10, w += (size_t) (c - '0'), c = fmt[++i];
      if (c == '.') {
        c = fmt[++i];
        if (c == '*') {
          pr = (size_t) va_arg(ap, int);
          c = fmt[++i];
        } else {
          while (is_digit(c)) pr *= 10, pr += (size_t) (c - '0'), c = fmt[++i];
        }
      }
      while (c == 'h') c = fmt[++i];  // Treat h and hh as int
      if (c == 'l') {
        is_long++, c = fmt[++i];
        if (c == 'l') is_long++, c = fmt[++i];
      }
      if (c == 'p') x = 1, is_long = 1;
      if (c == 'd' || c == 'u' || c == 'x' || c == 'X' || c == 'p') {
        bool s = (c == 'd'), h = (c == 'x' || c == 'X' || c == 'p');
        char tmp[30];
        size_t xl = x ? 2 : 0;
        if (is_long == 2) {
          int64_t v = va_arg(ap, int64_t);
          k = mg_lld(tmp, v, s, h);
        } else if (is_long == 1) {
          long v = va_arg(ap, long);
          k = mg_lld(tmp, s ? (int64_t) v : (int64_t) (unsigned long) v, s, h);
        } else {
          int v = va_arg(ap, int);
          k = mg_lld(tmp, s ? (int64_t) v : (int64_t) (unsigned) v, s, h);
        }
        for (j = 0; j < xl && w > 0; j++) w--;
        for (j = 0; pad == ' ' && !minus && k < w && j + k < w; j++)
          n += mg_copys(buf, len, n, &pad, 1);
        n += mg_copys(buf, len, n, (char *) "0x", xl);
        for (j = 0; pad == '0' && k < w && j + k < w; j++)
          n += mg_copys(buf, len, n, &pad, 1);
        n += mg_copys(buf, len, n, tmp, k);
        for (j = 0; pad == ' ' && minus && k < w && j + k < w; j++)
          n += mg_copys(buf, len, n, &pad, 1);
      } else if (c == 'c') {
        int p = va_arg(ap, int);
        if (n < len) buf[n] = (char) p;
        n++;
      } else if (c == 's') {
        char *p = va_arg(ap, char *);
        if (pr == 0) pr = p == NULL ? 0 : strlen(p);
        for (j = 0; !minus && pr < w && j + pr < w; j++)
          n += mg_copys(buf, len, n, &pad, 1);
        n += mg_copys(buf, len, n, p, pr);
        for (j = 0; minus && pr < w && j + pr < w; j++)
          n += mg_copys(buf, len, n, &pad, 1);
      } else if (c == '%') {
        if (n < len) buf[n] = '%';
        n++;
      } else {
        if (n < len) buf[n] = '%';
        n++;
        if (n < len) buf[n] = c;
        n++;
      }
      i++;
    } else {
      if (n < len) buf[n] = fmt[i];
      n++, i++;
    }
  }
  if (n < len) buf[n] = '\0';
  return n;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/timer.c"
#endif
// Copyright (c) Cesanta Software Limited
// All rights reserved




struct mg_timer *g_timers;

void mg_timer_init(struct mg_timer *t, int64_t ms, unsigned flags,
                   void (*fn)(void *), void *arg) {
  struct mg_timer tmp = {ms, 0UL, flags, fn, arg, g_timers};
  *t = tmp;
  g_timers = t;
  if (flags & MG_TIMER_RUN_NOW) fn(arg);
}

void mg_timer_free(struct mg_timer *t) {
  struct mg_timer **head = &g_timers;
  while (*head && *head != t) head = &(*head)->next;
  if (*head) *head = t->next;
}

void mg_timer_poll(int64_t now_ms) {
  // If time goes back (wrapped around), reset timers
  struct mg_timer *t, *tmp;
  static int64_t oldnow;  // Timestamp in a previous invocation
  if (oldnow > now_ms) {  // If it is wrapped, reset timers
    for (t = g_timers; t != NULL; t = t->next) t->expire = 0;
  }
  oldnow = now_ms;

  for (t = g_timers; t != NULL; t = tmp) {
    tmp = t->next;
    if (t->expire == 0) t->expire = now_ms + t->period_ms;
    if (t->expire > now_ms) continue;
    t->fn(t->arg);
    // Try to tick timers with the given period as accurate as possible,
    // even if this polling function is called with some random period.
    t->expire = now_ms - t->expire > t->period_ms ? now_ms + t->period_ms
                                                  : t->expire + t->period_ms;
    if (!(t->flags & MG_TIMER_REPEAT)) mg_timer_free(t);
  }
}

#ifdef MG_ENABLE_LINES
#line 1 "src/tls_dummy.c"
#endif


#if !MG_ENABLE_MBEDTLS && !MG_ENABLE_OPENSSL && !MG_ENABLE_CUSTOM_TLS
void mg_tls_init(struct mg_connection *c, struct mg_tls_opts *opts) {
  (void) opts;
  mg_error(c, "TLS is not enabled");
}
void mg_tls_handshake(struct mg_connection *c) {
  (void) c;
}
void mg_tls_free(struct mg_connection *c) {
  (void) c;
}
long mg_tls_recv(struct mg_connection *c, void *buf, size_t len) {
  return c == NULL || buf == NULL || len == 0 ? 0 : -1;
}
long mg_tls_send(struct mg_connection *c, const void *buf, size_t len) {
  return c == NULL || buf == NULL || len == 0 ? 0 : -1;
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/tls_mbed.c"
#endif



#if MG_ENABLE_MBEDTLS

#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000
#define MGRNG , rng_get, NULL
#else
#define MGRNG
#endif

void mg_tls_free(struct mg_connection *c) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  if (tls != NULL) {
    free(tls->cafile);
    mbedtls_ssl_free(&tls->ssl);
    mbedtls_pk_free(&tls->pk);
    mbedtls_x509_crt_free(&tls->ca);
    mbedtls_x509_crl_free(&tls->crl);
    mbedtls_x509_crt_free(&tls->cert);
    mbedtls_ssl_config_free(&tls->conf);
    free(tls);
    c->tls = NULL;
  }
}

bool mg_sock_would_block(void);

static int mg_net_send(void *ctx, const unsigned char *buf, size_t len) {
  struct mg_connection *c = (struct mg_connection *) ctx;
  int fd = (int) (size_t) c->fd;
  int n = (int) send(fd, buf, len, 0);
  MG_VERBOSE(("%lu n=%d, errno=%d", c->id, n, errno));
  if (n > 0) return n;
  if (n < 0 && mg_sock_would_block()) return MBEDTLS_ERR_SSL_WANT_WRITE;
  return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int mg_net_recv(void *ctx, unsigned char *buf, size_t len) {
  struct mg_connection *c = (struct mg_connection *) ctx;
  int n, fd = (int) (size_t) c->fd;
  n = (int) recv(fd, buf, len, 0);
  MG_VERBOSE(("%lu n=%d, errno=%d", c->id, n, errno));
  if (n > 0) return n;
  if (n < 0 && mg_sock_would_block()) return MBEDTLS_ERR_SSL_WANT_READ;
  return MBEDTLS_ERR_NET_RECV_FAILED;
}

void mg_tls_handshake(struct mg_connection *c) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int rc;
  mbedtls_ssl_set_bio(&tls->ssl, c, mg_net_send, mg_net_recv, 0);
  rc = mbedtls_ssl_handshake(&tls->ssl);
  if (rc == 0) {  // Success
    MG_DEBUG(("%lu success", c->id));
    c->is_tls_hs = 0;
  } else if (rc == MBEDTLS_ERR_SSL_WANT_READ ||
             rc == MBEDTLS_ERR_SSL_WANT_WRITE) {  // Still pending
    MG_VERBOSE(("%lu pending, %d%d %d (-%#x)", c->id, c->is_connecting,
                c->is_tls_hs, rc, -rc));
  } else {
    mg_error(c, "TLS handshake: -%#x", -rc);  // Error
  }
}

static int mbed_rng(void *ctx, unsigned char *buf, size_t len) {
  mg_random(buf, len);
  (void) ctx;
  return 0;
}

static void debug_cb(void *c, int lev, const char *s, int n, const char *s2) {
  n = (int) strlen(s2) - 1;
  MG_VERBOSE(("%lu %d %.*s", ((struct mg_connection *) c)->id, lev, n, s2));
  (void) s;
}

#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000
static int rng_get(void *p_rng, unsigned char *buf, size_t len) {
  (void) p_rng;
  mg_random(buf, len);
  return 0;
}
#endif

static struct mg_str mg_loadfile(struct mg_fs *fs, const char *path) {
  size_t n = 0;
  if (path[0] == '-') return mg_str(path);
  char *p = mg_file_read(fs, path, &n);
  return mg_str_n(p, n);
}

void mg_tls_init(struct mg_connection *c, struct mg_tls_opts *opts) {
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct mg_tls *tls = (struct mg_tls *) calloc(1, sizeof(*tls));
  int rc = 0;
  c->tls = tls;
  if (c->tls == NULL) {
    mg_error(c, "TLS OOM");
    goto fail;
  }
  MG_DEBUG(("%lu Setting TLS", c->id));
  mbedtls_ssl_init(&tls->ssl);
  mbedtls_ssl_config_init(&tls->conf);
  mbedtls_x509_crt_init(&tls->ca);
  mbedtls_x509_crl_init(&tls->crl);
  mbedtls_x509_crt_init(&tls->cert);
  mbedtls_pk_init(&tls->pk);
  mbedtls_ssl_conf_dbg(&tls->conf, debug_cb, c);
#if defined(MG_MBEDTLS_DEBUG_LEVEL)
  mbedtls_debug_set_threshold(MG_MBEDTLS_DEBUG_LEVEL);
#endif
  if ((rc = mbedtls_ssl_config_defaults(
           &tls->conf,
           c->is_client ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
           MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    mg_error(c, "tls defaults %#x", -rc);
    goto fail;
  }
  mbedtls_ssl_conf_rng(&tls->conf, mbed_rng, c);
  if (opts->ca == NULL || strcmp(opts->ca, "*") == 0) {
    mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_NONE);
  } else if (opts->ca != NULL && opts->ca[0] != '\0') {
    if (opts->crl != NULL && opts->crl[0] != '\0') {
      struct mg_str s = mg_loadfile(fs, opts->crl);
      rc = mbedtls_x509_crl_parse(&tls->crl, (uint8_t *) s.ptr, s.len + 1);
      if (opts->crl[0] != '-') free((char *) s.ptr);
      if (rc != 0) {
        mg_error(c, "parse(%s) err %#x", opts->crl, -rc);
        goto fail;
      }
    }
#if defined(MBEDTLS_X509_CA_CHAIN_ON_DISK)
    tls->cafile = strdup(opts->ca);
    rc = mbedtls_ssl_conf_ca_chain_file(&tls->conf, tls->cafile, &tls->crl);
    if (rc != 0) {
      mg_error(c, "parse on-disk chain(%s) err %#x", tls->cafile, -rc);
      goto fail;
    }
#else
    struct mg_str s = mg_loadfile(fs, opts->ca);
    rc = mbedtls_x509_crt_parse(&tls->ca, (uint8_t *) s.ptr, s.len + 1);
    if (opts->ca[0] != '-') free((char *) s.ptr);
    if (rc != 0) {
      mg_error(c, "parse(%s) err %#x", opts->ca, -rc);
      goto fail;
    }
    mbedtls_ssl_conf_ca_chain(&tls->conf, &tls->ca, &tls->crl);
#endif
    if (opts->srvname.len > 0) {
      char mem[128], *buf = mem;
      mg_asprintf(&buf, sizeof(mem), "%.*s", (int) opts->srvname.len,
                  opts->srvname.ptr);
      mbedtls_ssl_set_hostname(&tls->ssl, buf);
      if (buf != mem) free(buf);
    }
    mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  }
  if (opts->cert != NULL && opts->cert[0] != '\0') {
    struct mg_str s = mg_loadfile(fs, opts->cert);
    const char *key = opts->certkey == NULL ? opts->cert : opts->certkey;
    rc = mbedtls_x509_crt_parse(&tls->cert, (uint8_t *) s.ptr, s.len + 1);
    if (opts->cert[0] != '-') free((char *) s.ptr);
    if (rc != 0) {
      mg_error(c, "parse(%s) err %#x", opts->cert, -rc);
      goto fail;
    }
    s = mg_loadfile(fs, key);
    rc = mbedtls_pk_parse_key(&tls->pk, (uint8_t *) s.ptr, s.len + 1, NULL,
                              0 MGRNG);
    if (key[0] != '-') free((char *) s.ptr);
    if (rc != 0) {
      mg_error(c, "tls key(%s) %#x", key, -rc);
      goto fail;
    }
    rc = mbedtls_ssl_conf_own_cert(&tls->conf, &tls->cert, &tls->pk);
    if (rc != 0) {
      mg_error(c, "own cert %#x", -rc);
      goto fail;
    }
  }
  if ((rc = mbedtls_ssl_setup(&tls->ssl, &tls->conf)) != 0) {
    mg_error(c, "setup err %#x", -rc);
    goto fail;
  }
  c->tls = tls;
  c->is_tls = 1;
  c->is_tls_hs = 1;
  if (c->is_client && c->is_resolving == 0 && c->is_connecting == 0) {
    mg_tls_handshake(c);
  }
  return;
fail:
  mg_tls_free(c);
}

long mg_tls_recv(struct mg_connection *c, void *buf, size_t len) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  long n = mbedtls_ssl_read(&tls->ssl, (unsigned char *) buf, len);
  return n == 0 ? -1 : n == MBEDTLS_ERR_SSL_WANT_READ ? 0 : n;
}

long mg_tls_send(struct mg_connection *c, const void *buf, size_t len) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  long n = mbedtls_ssl_write(&tls->ssl, (unsigned char *) buf, len);
  return n == 0 ? -1 : n == MBEDTLS_ERR_SSL_WANT_WRITE ? 0 : n;
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/tls_openssl.c"
#endif


#if MG_ENABLE_OPENSSL
static int mg_tls_err(struct mg_tls *tls, int res) {
  int err = SSL_get_error(tls->ssl, res);
  // We've just fetched the last error from the queue.
  // Now we need to clear the error queue. If we do not, then the following
  // can happen (actually reported):
  //  - A new connection is accept()-ed with cert error (e.g. self-signed cert)
  //  - Since all accept()-ed connections share listener's context,
  //  - *ALL* SSL accepted connection report read error on the next poll cycle.
  //    Thus a single errored connection can close all the rest, unrelated ones.
  // Clearing the error keeps the shared SSL_CTX in an OK state.

  if (err != 0) ERR_print_errors_fp(stderr);
  ERR_clear_error();
  if (err == SSL_ERROR_WANT_READ) return 0;
  if (err == SSL_ERROR_WANT_WRITE) return 0;
  return err;
}

void mg_tls_init(struct mg_connection *c, struct mg_tls_opts *opts) {
  struct mg_tls *tls = (struct mg_tls *) calloc(1, sizeof(*tls));
  const char *id = "mongoose";
  static unsigned char s_initialised = 0;
  int rc;

  if (tls == NULL) {
    mg_error(c, "TLS OOM");
    goto fail;
  }

  if (!s_initialised) {
    SSL_library_init();
    s_initialised++;
  }
  MG_DEBUG(("%lu Setting TLS, CA: %s, cert: %s, key: %s", c->id,
            opts->ca == NULL ? "null" : opts->ca,
            opts->cert == NULL ? "null" : opts->cert,
            opts->certkey == NULL ? "null" : opts->certkey));
  tls->ctx = c->is_client ? SSL_CTX_new(SSLv23_client_method())
                          : SSL_CTX_new(SSLv23_server_method());
  if ((tls->ssl = SSL_new(tls->ctx)) == NULL) {
    mg_error(c, "SSL_new");
    goto fail;
  }
  SSL_set_session_id_context(tls->ssl, (const uint8_t *) id,
                             (unsigned) strlen(id));
  // Disable deprecated protocols
  SSL_set_options(tls->ssl, SSL_OP_NO_SSLv2);
  SSL_set_options(tls->ssl, SSL_OP_NO_SSLv3);
  SSL_set_options(tls->ssl, SSL_OP_NO_TLSv1);
  SSL_set_options(tls->ssl, SSL_OP_NO_TLSv1_1);
#ifdef MG_ENABLE_OPENSSL_NO_COMPRESSION
  SSL_set_options(tls->ssl, SSL_OP_NO_COMPRESSION);
#endif
#ifdef MG_ENABLE_OPENSSL_CIPHER_SERVER_PREFERENCE
  SSL_set_options(tls->ssl, SSL_OP_CIPHER_SERVER_PREFERENCE);
#endif

  if (opts->ca != NULL && opts->ca[0] != '\0') {
    SSL_set_verify(tls->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                   NULL);
    if ((rc = SSL_CTX_load_verify_locations(tls->ctx, opts->ca, NULL)) != 1) {
      mg_error(c, "parse(%s): err %d", opts->ca, mg_tls_err(tls, rc));
      goto fail;
    }
  }
  if (opts->cert != NULL && opts->cert[0] != '\0') {
    const char *key = opts->certkey;
    if (key == NULL) key = opts->cert;
    if ((rc = SSL_use_certificate_file(tls->ssl, opts->cert, 1)) != 1) {
      mg_error(c, "Invalid SSL cert, err %d", mg_tls_err(tls, rc));
      goto fail;
    } else if ((rc = SSL_use_PrivateKey_file(tls->ssl, key, 1)) != 1) {
      mg_error(c, "Invalid SSL key, err %d", mg_tls_err(tls, rc));
      goto fail;
#if OPENSSL_VERSION_NUMBER > 0x10100000L
    } else if ((rc = SSL_use_certificate_chain_file(tls->ssl, opts->cert)) !=
               1) {
      mg_error(c, "Invalid CA, err %d", mg_tls_err(tls, rc));
      goto fail;
#endif
    } else {
      SSL_set_mode(tls->ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#if OPENSSL_VERSION_NUMBER > 0x10002000L
      SSL_set_ecdh_auto(tls->ssl, 1);
#endif
    }
  }
  if (opts->srvname.len > 0) {
    char mem[128], *buf = mem;
    size_t len = mg_asprintf(&buf, sizeof(mem), "%.*s", (int) opts->srvname.len,
                             opts->srvname.ptr);
    X509_VERIFY_PARAM_set1_host(SSL_get0_param(tls->ssl), buf, len);
    if (buf != mem) free(buf);
  }
  if (opts->ciphers != NULL) SSL_set_cipher_list(tls->ssl, opts->ciphers);
  if (opts->srvname.len > 0) {
    char mem[128], *buf = mem;
    mg_asprintf(&buf, sizeof(mem), "%.*s", (int) opts->srvname.len,
                opts->srvname.ptr);
    SSL_set_tlsext_host_name(tls->ssl, buf);
    if (buf != mem) free(buf);
  }
  c->tls = tls;
  c->is_tls = 1;
  c->is_tls_hs = 1;
  if (c->is_client && c->is_resolving == 0 && c->is_connecting == 0) {
    mg_tls_handshake(c);
  }
  MG_DEBUG(("%lu SSL %s OK", c->id, c->is_accepted ? "accept" : "client"));
  return;
fail:
  c->is_closing = 1;
  free(tls);
}

void mg_tls_handshake(struct mg_connection *c) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int rc;
  SSL_set_fd(tls->ssl, (int) (size_t) c->fd);
  rc = c->is_client ? SSL_connect(tls->ssl) : SSL_accept(tls->ssl);
  if (rc == 1) {
    MG_DEBUG(("%lu success", c->id));
    c->is_tls_hs = 0;
  } else {
    int code = mg_tls_err(tls, rc);
    if (code != 0) mg_error(c, "tls hs: rc %d, err %d", rc, code);
  }
}

void mg_tls_free(struct mg_connection *c) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  if (tls == NULL) return;
  SSL_free(tls->ssl);
  SSL_CTX_free(tls->ctx);
  free(tls);
  c->tls = NULL;
}

long mg_tls_recv(struct mg_connection *c, void *buf, size_t len) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int n = SSL_read(tls->ssl, buf, (int) len);
  return n == 0 ? -1 : n < 0 && mg_tls_err(tls, n) == 0 ? 0 : n;
}

long mg_tls_send(struct mg_connection *c, const void *buf, size_t len) {
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int n = SSL_write(tls->ssl, buf, (int) len);
  return n == 0 ? -1 : n < 0 && mg_tls_err(tls, n) == 0 ? 0 : n;
}
#endif

#ifdef MG_ENABLE_LINES
#line 1 "src/url.c"
#endif

#include <stdlib.h>

struct url {
  size_t key, user, pass, host, port, uri, end;
};

int mg_url_is_ssl(const char *url) {
  return strncmp(url, "wss:", 4) == 0 || strncmp(url, "https:", 6) == 0 ||
         strncmp(url, "mqtts:", 6) == 0 || strncmp(url, "ssl:", 4) == 0 ||
         strncmp(url, "tls:", 4) == 0;
}

static struct url urlparse(const char *url) {
  size_t i;
  struct url u;
  memset(&u, 0, sizeof(u));
  for (i = 0; url[i] != '\0'; i++) {
    if (i > 0 && u.host == 0 && url[i - 1] == '/' && url[i] == '/') {
      u.host = i + 1;
      u.port = 0;
    } else if (url[i] == ']') {
      u.port = 0;  // IPv6 URLs, like http://[::1]/bar
    } else if (url[i] == ':' && u.port == 0 && u.uri == 0) {
      u.port = i + 1;
    } else if (url[i] == '@' && u.user == 0 && u.pass == 0) {
      u.user = u.host;
      u.pass = u.port;
      u.host = i + 1;
      u.port = 0;
    } else if (u.host && u.uri == 0 && url[i] == '/') {
      u.uri = i;
    }
  }
  u.end = i;
#if 0
  printf("[%s] %d %d %d %d %d\n", url, u.user, u.pass, u.host, u.port, u.uri);
#endif
  return u;
}

struct mg_str mg_url_host(const char *url) {
  struct url u = urlparse(url);
  size_t n = u.port  ? u.port - u.host - 1
             : u.uri ? u.uri - u.host
                     : u.end - u.host;
  struct mg_str s = mg_str_n(url + u.host, n);
  return s;
}

const char *mg_url_uri(const char *url) {
  struct url u = urlparse(url);
  return u.uri ? url + u.uri : "/";
}

unsigned short mg_url_port(const char *url) {
  struct url u = urlparse(url);
  unsigned short port = 0;
  if (strncmp(url, "http:", 5) == 0 || strncmp(url, "ws:", 3) == 0) port = 80;
  if (strncmp(url, "wss:", 4) == 0 || strncmp(url, "https:", 6) == 0)
    port = 443;
  if (strncmp(url, "mqtt:", 5) == 0) port = 1883;
  if (strncmp(url, "mqtts:", 6) == 0) port = 8883;
  if (u.port) port = (unsigned short) atoi(url + u.port);
  return port;
}

struct mg_str mg_url_user(const char *url) {
  struct url u = urlparse(url);
  struct mg_str s = mg_str("");
  if (u.user && (u.pass || u.host)) {
    size_t n = u.pass ? u.pass - u.user - 1 : u.host - u.user - 1;
    s = mg_str_n(url + u.user, n);
  }
  return s;
}

struct mg_str mg_url_pass(const char *url) {
  struct url u = urlparse(url);
  struct mg_str s = mg_str_n("", 0UL);
  if (u.pass && u.host) {
    size_t n = u.host - u.pass - 1;
    s = mg_str_n(url + u.pass, n);
  }
  return s;
}

#ifdef MG_ENABLE_LINES
#line 1 "src/util.c"
#endif


#if MG_ARCH == MG_ARCH_UNIX && defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if MG_ENABLE_CUSTOM_RANDOM
#else
void mg_random(void *buf, size_t len) {
  bool done = false;
  unsigned char *p = (unsigned char *) buf;
#if MG_ARCH == MG_ARCH_ESP32
  while (len--) *p++ = (unsigned char) (esp_random() & 255);
  done = true;
#elif MG_ARCH == MG_ARCH_WIN32
#elif MG_ARCH == MG_ARCH_UNIX
  FILE *fp = fopen("/dev/urandom", "rb");
  if (fp != NULL) {
    if (fread(buf, 1, len, fp) == len) done = true;
    fclose(fp);
  }
#endif
  // If everything above did not work, fallback to a pseudo random generator
  while (!done && len--) *p++ = (unsigned char) (rand() & 255);
}
#endif

uint32_t mg_ntohl(uint32_t net) {
  uint8_t data[4] = {0, 0, 0, 0};
  memcpy(&data, &net, sizeof(data));
  return (((uint32_t) data[3]) << 0) | (((uint32_t) data[2]) << 8) |
         (((uint32_t) data[1]) << 16) | (((uint32_t) data[0]) << 24);
}

uint16_t mg_ntohs(uint16_t net) {
  uint8_t data[2] = {0, 0};
  memcpy(&data, &net, sizeof(data));
  return (uint16_t) ((uint16_t) data[1] | (((uint16_t) data[0]) << 8));
}

uint32_t mg_crc32(uint32_t crc, const char *buf, size_t len) {
  int i;
  crc = ~crc;
  while (len--) {
    crc ^= *(unsigned char *) buf++;
    for (i = 0; i < 8; i++) crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
  }
  return ~crc;
}

static int isbyte(int n) {
  return n >= 0 && n <= 255;
}

static int parse_net(const char *spec, uint32_t *net, uint32_t *mask) {
  int n, a, b, c, d, slash = 32, len = 0;
  if ((sscanf(spec, "%d.%d.%d.%d/%d%n", &a, &b, &c, &d, &slash, &n) == 5 ||
       sscanf(spec, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4) &&
      isbyte(a) && isbyte(b) && isbyte(c) && isbyte(d) && slash >= 0 &&
      slash < 33) {
    len = n;
    *net = ((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) |
           (uint32_t) d;
    *mask = slash ? (uint32_t) (0xffffffffU << (32 - slash)) : (uint32_t) 0;
  }
  return len;
}

int mg_check_ip_acl(struct mg_str acl, uint32_t remote_ip) {
  struct mg_str k, v;
  int allowed = acl.len == 0 ? '+' : '-';  // If any ACL is set, deny by default
  while (mg_commalist(&acl, &k, &v)) {
    uint32_t net, mask;
    if (k.ptr[0] != '+' && k.ptr[0] != '-') return -1;
    if (parse_net(&k.ptr[1], &net, &mask) == 0) return -2;
    if ((mg_ntohl(remote_ip) & mask) == net) allowed = k.ptr[0];
  }
  return allowed == '+';
}

int64_t mg_millis(void) {
#if MG_ARCH == MG_ARCH_WIN32
  return GetTickCount();
#elif MG_ARCH == MG_ARCH_ESP32
  return esp_timer_get_time() / 1000;
#elif MG_ARCH == MG_ARCH_ESP8266
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
#elif MG_ARCH == MG_ARCH_FREERTOS_TCP || MG_ARCH == MG_ARCH_FREERTOS_LWIP
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
#elif MG_ARCH == MG_ARCH_AZURERTOS
  return tx_time_get() * (1000 /* MS per SEC */ / TX_TIMER_TICKS_PER_SECOND);
#elif MG_ARCH == MG_ARCH_UNIX && defined(__APPLE__)
  uint64_t ticks = mach_absolute_time();
  static mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  double ticks_to_nanos = (double) timebase.numer / timebase.denom;
  uint64_t uptime_nanos = (uint64_t) (ticks_to_nanos * ticks);
  return (int64_t) (uptime_nanos / 1000000);
#elif MG_ARCH == MG_ARCH_UNIX
  struct timespec ts;
#ifdef _POSIX_MONOTONIC_CLOCK
#ifdef CLOCK_MONOTONIC_RAW
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  return ((int64_t) ts.tv_sec * 1000 + (int64_t) ts.tv_nsec / 1000000);
#else
  return time(NULL) * 1000;
#endif
}

#ifdef MG_ENABLE_LINES
#line 1 "src/ws.c"
#endif









struct ws_msg {
  uint8_t flags;
  size_t header_len;
  size_t data_len;
};

static void ws_handshake(struct mg_connection *c, const struct mg_str *wskey,
                         const struct mg_str *wsproto, const char *fmt,
                         va_list ap) {
  const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  unsigned char sha[20], b64_sha[30];
  char mem[128], *buf = mem;

  mg_sha1_ctx sha_ctx;
  mg_sha1_init(&sha_ctx);
  mg_sha1_update(&sha_ctx, (unsigned char *) wskey->ptr, wskey->len);
  mg_sha1_update(&sha_ctx, (unsigned char *) magic, 36);
  mg_sha1_final(sha, &sha_ctx);
  mg_base64_encode(sha, sizeof(sha), (char *) b64_sha);
  buf[0] = '\0';
  if (fmt != NULL) mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  mg_printf(c,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "%s",
            b64_sha, buf);
  if (buf != mem) free(buf);
  if (wsproto != NULL) {
    mg_printf(c, "Sec-WebSocket-Protocol: %.*s\r\n", (int) wsproto->len,
              wsproto->ptr);
  }
  mg_send(c, "\r\n", 2);
}

static size_t ws_process(uint8_t *buf, size_t len, struct ws_msg *msg) {
  size_t i, n = 0, mask_len = 0;
  memset(msg, 0, sizeof(*msg));
  if (len >= 2) {
    n = buf[1] & 0x7f;                // Frame length
    mask_len = buf[1] & 128 ? 4 : 0;  // last bit is a mask bit
    msg->flags = buf[0];
    if (n < 126 && len >= mask_len) {
      msg->data_len = n;
      msg->header_len = 2 + mask_len;
    } else if (n == 126 && len >= 4 + mask_len) {
      msg->header_len = 4 + mask_len;
      msg->data_len = mg_ntohs(*(uint16_t *) &buf[2]);
    } else if (len >= 10 + mask_len) {
      msg->header_len = 10 + mask_len;
      msg->data_len =
          (size_t) (((uint64_t) mg_ntohl(*(uint32_t *) &buf[2])) << 32) +
          mg_ntohl(*(uint32_t *) &buf[6]);
    }
  }
  // Sanity check, and integer overflow protection for the boundary check below
  // data_len should not be larger than 1 Gb
  if (msg->data_len > 1024 * 1024 * 1024) return 0;
  if (msg->header_len + msg->data_len > len) return 0;
  if (mask_len > 0) {
    uint8_t *p = buf + msg->header_len, *m = p - mask_len;
    for (i = 0; i < msg->data_len; i++) p[i] ^= m[i & 3];
  }
  return msg->header_len + msg->data_len;
}

static size_t mkhdr(size_t len, int op, bool is_client, uint8_t *buf) {
  size_t n = 0;
  buf[0] = (uint8_t) (op | 128);
  if (len < 126) {
    buf[1] = (unsigned char) len;
    n = 2;
  } else if (len < 65536) {
    uint16_t tmp = mg_htons((uint16_t) len);
    buf[1] = 126;
    memcpy(&buf[2], &tmp, sizeof(tmp));
    n = 4;
  } else {
    uint32_t tmp;
    buf[1] = 127;
    tmp = mg_htonl((uint32_t) ((uint64_t) len >> 32));
    memcpy(&buf[2], &tmp, sizeof(tmp));
    tmp = mg_htonl((uint32_t) (len & 0xffffffff));
    memcpy(&buf[6], &tmp, sizeof(tmp));
    n = 10;
  }
  if (is_client) {
    buf[1] |= 1 << 7;  // Set masking flag
    mg_random(&buf[n], 4);
    n += 4;
  }
  return n;
}

static void mg_ws_mask(struct mg_connection *c, size_t len) {
  if (c->is_client && c->send.buf != NULL) {
    size_t i;
    uint8_t *p = c->send.buf + c->send.len - len, *mask = p - 4;
    for (i = 0; i < len; i++) p[i] ^= mask[i & 3];
  }
}

size_t mg_ws_send(struct mg_connection *c, const char *buf, size_t len,
                  int op) {
  uint8_t header[14];
  size_t header_len = mkhdr(len, op, c->is_client, header);
  mg_send(c, header, header_len);
  MG_VERBOSE(("WS out: %d [%.*s]", (int) len, (int) len, buf));
  mg_send(c, buf, len);
  mg_ws_mask(c, len);
  return header_len + len;
}

static void mg_ws_cb(struct mg_connection *c, int ev, void *ev_data,
                     void *fn_data) {
  struct ws_msg msg;
  size_t ofs = (size_t) c->pfn_data;

  // assert(ofs < c->recv.len);
  if (ev == MG_EV_READ) {
    if (!c->is_websocket && c->is_client) {
      int n = mg_http_get_request_len(c->recv.buf, c->recv.len);
      if (n < 0) {
        c->is_closing = 1;  // Some just, not an HTTP request
      } else if (n > 0) {
        if (n < 15 || memcmp(c->recv.buf + 9, "101", 3) != 0) {
          MG_ERROR(("%lu WS handshake error: %.*s", c->id, 15, c->recv.buf));
          c->is_closing = 1;
        } else {
          struct mg_http_message hm;
          mg_http_parse((char *) c->recv.buf, c->recv.len, &hm);
          c->is_websocket = 1;
          mg_call(c, MG_EV_WS_OPEN, &hm);
        }
        mg_iobuf_del(&c->recv, 0, (size_t) n);
      } else {
        return;  // A request is not yet received
      }
    }

    while (ws_process(c->recv.buf + ofs, c->recv.len - ofs, &msg) > 0) {
      char *s = (char *) c->recv.buf + ofs + msg.header_len;
      struct mg_ws_message m = {{s, msg.data_len}, msg.flags};
      size_t len = msg.header_len + msg.data_len;
      uint8_t final = msg.flags & 128, op = msg.flags & 15;
      // MG_VERBOSE ("fin %d op %d len %d [%.*s]", final, op,
      //                       (int) m.data.len, (int) m.data.len, m.data.ptr));
      switch (op) {
        case WEBSOCKET_OP_CONTINUE:
          mg_call(c, MG_EV_WS_CTL, &m);
          break;
        case WEBSOCKET_OP_PING:
          MG_DEBUG(("%s", "WS PONG"));
          mg_ws_send(c, s, msg.data_len, WEBSOCKET_OP_PONG);
          mg_call(c, MG_EV_WS_CTL, &m);
          break;
        case WEBSOCKET_OP_PONG:
          mg_call(c, MG_EV_WS_CTL, &m);
          break;
        case WEBSOCKET_OP_TEXT:
        case WEBSOCKET_OP_BINARY:
          if (final) mg_call(c, MG_EV_WS_MSG, &m);
          break;
        case WEBSOCKET_OP_CLOSE:
          MG_DEBUG(("%lu Got WS CLOSE", c->id));
          mg_call(c, MG_EV_WS_CTL, &m);
          c->is_closing = 1;
          break;
        default:
          // Per RFC6455, close conn when an unknown op is recvd
          mg_error(c, "unknown WS op %d", op);
          break;
      }

      // Handle fragmented frames: strip header, keep in c->recv
      if (final == 0 || op == 0) {
        if (op) ofs++, len--, msg.header_len--;       // First frame
        mg_iobuf_del(&c->recv, ofs, msg.header_len);  // Strip header
        len -= msg.header_len;
        ofs += len;
        c->pfn_data = (void *) ofs;
        // MG_INFO(("FRAG %d [%.*s]", (int) ofs, (int) ofs, c->recv.buf));
      }
      // Remove non-fragmented frame
      if (final && op) mg_iobuf_del(&c->recv, ofs, len);
      // Last chunk of the fragmented frame
      if (final && !op) {
        m.flags = c->recv.buf[0];
        m.data = mg_str_n((char *) &c->recv.buf[1], (size_t) (ofs - 1));
        mg_call(c, MG_EV_WS_MSG, &m);
        mg_iobuf_del(&c->recv, 0, ofs);
        ofs = 0;
        c->pfn_data = NULL;
      }
    }
  }
  (void) fn_data;
  (void) ev_data;
}

struct mg_connection *mg_ws_connect(struct mg_mgr *mgr, const char *url,
                                    mg_event_handler_t fn, void *fn_data,
                                    const char *fmt, ...) {
  struct mg_connection *c = mg_connect(mgr, url, fn, fn_data);
  if (c != NULL) {
    char nonce[16], key[30], mem1[128], mem2[256], *buf1 = mem1, *buf2 = mem2;
    struct mg_str host = mg_url_host(url);
    size_t n1 = 0, n2 = 0;
    nonce[0] = key[0] = mem1[0] = mem2[0] = '\0';
    if (fmt != NULL) {
      va_list ap;
      va_start(ap, fmt);
      n1 = mg_vasprintf(&buf1, sizeof(mem1), fmt, ap);
      va_end(ap);
    }
    // Send handshake request
    mg_random(nonce, sizeof(nonce));
    mg_base64_encode((unsigned char *) nonce, sizeof(nonce), key);
    n2 = mg_asprintf(&buf2, sizeof(mem2),
                     "GET %s HTTP/1.1\r\n"
                     "Upgrade: websocket\r\n"
                     "Host: %.*s\r\n"
                     "Connection: Upgrade\r\n"
                     "%.*s"
                     "Sec-WebSocket-Version: 13\r\n"
                     "Sec-WebSocket-Key: %s\r\n"
                     "\r\n",
                     mg_url_uri(url), (int) host.len, host.ptr, (int) n1, buf1,
                     key);
    mg_send(c, buf2, n2);
    if (buf1 != mem1) free(buf1);
    if (buf2 != mem2) free(buf2);
    c->pfn = mg_ws_cb;
    c->pfn_data = NULL;
  }
  return c;
}

void mg_ws_upgrade(struct mg_connection *c, struct mg_http_message *hm,
                   const char *fmt, ...) {
  struct mg_str *wskey = mg_http_get_header(hm, "Sec-WebSocket-Key");
  c->pfn = mg_ws_cb;
  c->pfn_data = NULL;
  if (wskey == NULL) {
    mg_http_reply(c, 426, "", "WS upgrade expected\n");
    c->is_draining = 1;
  } else {
    struct mg_str *wsproto = mg_http_get_header(hm, "Sec-WebSocket-Protocol");
    va_list ap;
    va_start(ap, fmt);
    ws_handshake(c, wskey, wsproto, fmt, ap);
    va_end(ap);
    c->is_websocket = 1;
    mg_call(c, MG_EV_WS_OPEN, hm);
  }
}

size_t mg_ws_wrap(struct mg_connection *c, size_t len, int op) {
  uint8_t header[14], *p;
  size_t header_len = mkhdr(len, op, c->is_client, header);

  // NOTE: order of operations is important!
  mg_iobuf_add(&c->send, c->send.len, NULL, header_len, MG_IO_SIZE);
  p = &c->send.buf[c->send.len - len];         // p points to data
  memmove(p, p - header_len, len);             // Shift data
  memcpy(p - header_len, header, header_len);  // Prepend header
  mg_ws_mask(c, len);                          // Mask data

  return c->send.len;
}
