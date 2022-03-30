// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"

static const char *s_debug_level = "3";
static const char *s_root_dir = ".";
static const char *s_listening_address = "http://0.0.0.0:8000";
static time_t s_boot_timestamp = 0;
static struct mg_connection *s_sntp_conn = NULL;

// Event handler for the listening connection.
// Simply serve static files from `s_root_dir`
static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data, tmp = {0};
    struct mg_str unknown = mg_str_n("?", 1), *cl;
    struct mg_http_serve_opts opts = {0};
    opts.root_dir = s_root_dir;
    mg_http_serve_dir(c, hm, &opts);
    mg_http_parse((char *) c->send.buf, c->send.len, &tmp);
    cl = mg_http_get_header(&tmp, "Content-Length");
    if (cl == NULL) cl = &unknown;
    MG_INFO(("%.*s %.*s %.*s %.*s", (int) hm->method.len, hm->method.ptr,
             (int) hm->uri.len, hm->uri.ptr, (int) tmp.uri.len, tmp.uri.ptr,
             (int) cl->len, cl->ptr));
  } else if (ev == MG_EV_OPEN) {
    c->is_hexdumping = 1;
  }
  (void) fn_data;
}

// We have no valid system time(), and we need it for TLS. Implement it
time_t time(time_t *tp) {
  time_t t = s_boot_timestamp + mg_millis() / 1000;
  if (tp != NULL) *tp = t;
  return t;
}

static void sfn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_SNTP_TIME) {
    int64_t t = *(int64_t *) ev_data;
    MG_INFO(("Got SNTP time: %lld ms from epoch", t));
    s_boot_timestamp = (time_t) ((t - mg_millis()) / 1000);
  } else if (ev == MG_EV_CLOSE) {
    s_sntp_conn = NULL;
  }
}

static void timer_fn(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  if (s_sntp_conn == NULL) s_sntp_conn = mg_sntp_connect(mgr, NULL, sfn, NULL);
  if (s_boot_timestamp < 9999) mg_sntp_send(s_sntp_conn, time(NULL));
}

static void logfn(const void *ptr, size_t len, void *userdata) {
  // printk("%.*s", (int) len, (char *) ptr);
  printk("Press the button\n");
}

int main(int argc, char *argv[]) {
  struct mg_mgr mgr;

  mg_log_set(s_debug_level);
  mg_log_set_callback(logfn, NULL);
  MG_INFO(("hi there..."));
  printk("booo\n");
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, s_listening_address, cb, &mgr);

  struct mg_timer t;
  mg_timer_init(&t, 5000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, timer_fn, &mgr);

  // Start infinite event loop
  MG_INFO(("Mongoose version : v%s", MG_VERSION));
  MG_INFO(("Listening on     : %s", s_listening_address));
  MG_INFO(("Web root         : [%s]", s_root_dir));
  for (;;) mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  return 0;
}
