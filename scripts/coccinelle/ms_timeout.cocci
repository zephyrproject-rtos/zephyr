// Copyright (c) 2019-2020 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Replace use of K_NO_WAIT and K_FOREVER in API that requires
// timeouts be specified as integral milliseconds.
//
// These constants used to have the values 0 and -1 respectively; they
// are now timeout values which are opaque non-integral values that
// can't be converted to integers automatically.  For K_NO_WAIT replace
// with 0; for K_FOREVER replace with SYS_FOREVER_MS, the value of
// which is -1.
//
// Options: --include-headers

virtual patch
virtual report

// ** Handle millisecond timeout as the last parameter

// Match identifer passed as timeout
@match_fn_l1@
identifier fn =~ "(?x)^
(dmic_read
|bt_buf_get_(rx|cmd_complete|evt)
|bt_mesh_cfg_cli_timeout_set
|isotp_(bind|recv(_net)?|send(_net)?(_ctx)?_buf)
|tty_set_(tx|rx)_timeout
|can_(write|recover)
|uart_(tx|rx_enable)
|dns_(resolve_name|get_addr_info)
|net_config_init
|net_ppp_ping
|websocket_(send|recv)_msg
)$";
identifier T;
@@
 fn(..., T);

@report_fn_l1
 extends match_fn_l1
 depends on report
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
position p;
@@
 fn@p(...,
(
K_NO_WAIT
|
K_FOREVER
)
 )

@script:python
 depends on report
@
fn << match_fn_l1.fn;
T << match_fn_l1.T;
p << report_fn_l1.p;
@@
msg = "WARNING: [msl1] replace constant {} with ms duration in {}".format(T, fn)
coccilib.report.print_report(p[0], msg);

@fix_fn_l1
 extends match_fn_l1
 depends on patch
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
@@
 fn(...,
(
- K_NO_WAIT
+ 0
|
- K_FOREVER
+ SYS_FOREVER_MS
))

// ** Handle millisecond timeout as second from last parameter

// Match identifer passed as timeout
@match_fn_l2@
identifier fn =~ "(?x)^
(http_client_req
|websocket_connect
)$";
expression L1;
identifier T;
@@
 fn(..., T, L1)

@report_fn_l2
 extends match_fn_l2
 depends on report
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
expression X1;
position p;
@@
 fn@p(...,
(
K_NO_WAIT
|
K_FOREVER
)
 , X1)

@script:python
 depends on report
@
fn << match_fn_l2.fn;
T << match_fn_l2.T;
p << report_fn_l2.p;
@@
msg = "WARNING: [msl2] replace constant {} with ms duration in {}".format(T, fn)
coccilib.report.print_report(p[0], msg);

@fix_fn_l2
 extends match_fn_l2
 depends on patch
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
expression X1;
@@
 fn(...,
(
- K_NO_WAIT
+ 0
|
- K_FOREVER
+ SYS_FOREVER_MS
)
 , X1)

// ** Handle millisecond timeout as third from last parameter

// Match identifer passed as timeout
@match_fn_l3@
identifier fn =~ "(?x)^
(can_send
|lora_recv
)$";
expression L1;
expression L2;
identifier T;
@@
 fn(..., T, L2, L1)

@report_fn_l3
 extends match_fn_l3
 depends on report
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
position p;
@@
 fn@p(...,
(
K_NO_WAIT
|
K_FOREVER
)
 , L2, L1)

@script:python
 depends on report
@
fn << match_fn_l3.fn;
T << match_fn_l3.T;
p << report_fn_l3.p;
@@
msg = "WARNING: [msl3] replace constant {} with ms duration in {}".format(T, fn)
coccilib.report.print_report(p[0], msg);

@fix_fn_l3
 extends match_fn_l3
 depends on patch
@
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
@@
 fn(...,
(
- K_NO_WAIT
+ 0
|
- K_FOREVER
+ SYS_FOREVER_MS
)
 , L2, L1)
