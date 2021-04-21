// Copyright (c) 2020 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Enforce preservation of const qualifier on config_info casts
//
// Drivers cast the device config_info pointer to a driver-specific
// structure.  The object is const-qualified; make sure the cast
// doesn't inadvertently remove that qualifier.
//
// Also add the qualifier to pointer definitions where it's missing.
//
// Note that this patch may produce incorrect results if config_info
// appears as a tag in non-device aggregate types.
//
// Options: --include-headers

virtual patch
virtual report

// bare: (struct T*)E
@r_cci_bare_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
expression E;
@@
 (
+const
 struct T*)E->config_info

// bare const: (struct T* const)E
@r_cci_bare_lc_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
expression E;
@@
 (
+const
 struct T * const)E->config_info

// asg: struct T *D = (const struct T*)
@r_cci_asg_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * D = (const struct T*)E->config_info;

// asg to const local: struct T * const D = (const struct T*)
@r_cci_lc_asg_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * const D = (const struct T*)E->config_info;

// asg via macro: struct T * D = DEV_CFG()
@r_cci_asg_macro_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * D = DEV_CFG(E);

// asg via macro to const local: struct T * const D = DEV_CFG()
@r_cci_lc_asg_macro_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * const D = DEV_CFG(E);

// asg via macro: struct T * D; ... ; D = (const struct T*)CI;
@r_cci_delayed_asg_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * D;
 ...
 D = (const struct T*)E->config_info;

// delayed asg via macro: struct T * D; ... ; D = DEV_CFG();
@r_cci_delayed_asg_macro_patch
 depends on patch
 disable optional_qualifier
@
identifier T;
identifier D;
expression E;
@@
+const
 struct T * D;
 ...
 D = DEV_CFG(E);

@r_cci_report
 depends on report
 disable optional_qualifier
@
identifier T;
expression E;
position p;
@@
 (struct T*)E->config_info@p

@script:python
 depends on report
@
t << r_cci_report.T;
p << r_cci_report.p;
@@
msg = "WARNING: cast of config_info to struct {} requires 'const'".format(t)
coccilib.report.print_report(p[0], msg)
