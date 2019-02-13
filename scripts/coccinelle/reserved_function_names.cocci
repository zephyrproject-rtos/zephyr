/// Find function names starting with '__' or '_' and rename them
// Copyright: (C) 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
// Options: --include-headers

virtual patch
virtual report
virtual changes

@r_anyfn disable optional_storage@
identifier fn =~ "^_[^_]+";
position p;
type T;
@@

(
  extern T fn@p (...);
|
  T fn@p (...);
|
  fn@p (...)
)

@r_staticfn@
identifier fn =~ "^_[^_]+";
position p;
type T;
@@

  static T fn@p (...) {...}

@script:python r_static_rewrite@
fn << r_staticfn.fn;
pos << r_staticfn.p;
z;
@@

ignore = ( '_Pragma',
           '_pragma',
	   '_Static',
	   '_CONCAT',
	   '_DO_CONCAT' )

prefixes = { '_arch_': 'z_arch_',
             '_sys_': 'z_sys_',
	     '_k_': 'z_',
	     '_K_': 'Z_',
	     '_impl_': 'z_impl_',
	     '_handler_': 'z_handl_',
	     '_ASSERT': 'Z_ASSERT',
	     '_Cstart': 'z_cstart',
	     '_Swap': 'z_swap',
	     '_ClearFaults': 'z_clearfaults' }

r = fn

dirstr = "/" + pos[0].file
if dirstr.find("/ext/") > -1:
    cocci.include_match(False)

for p in ignore:
    if r.startswith(p):
        cocci.include_match(False)

if dirstr.find("/include/") > -1:
    for p in prefixes:
        if r.startswith(p):
            r = r[len(p):]

	    while r.startswith('_'):
	        r = r[1:]

	    if r:
	        if prefixes[p].endswith('_'):
	            r = prefixes[p] + r
	        else:
	            r = prefixes[p] + "_" + r
            else:
	        r = prefixes[p]

	    break

else:
    while r.startswith('_'):
        r = r[1:]

coccinelle.z = r

@script:python r_any_rewrite@
fn << r_anyfn.fn;
pos << r_anyfn.p;
z;
@@

ignore = ( '_Pragma',
           '_pragma',
	   '_Static',
	   '_CONCAT',
	   '_DO_CONCAT' )

prefixes = { '_arch_': 'z_arch_',
             '_sys_': 'z_sys_',
	     '_k_': 'z_',
	     '_K_': 'Z_',
	     '_impl_': 'z_impl_',
	     '_handler_': 'z_handl_',
	     '_ASSERT': 'Z_ASSERT',
	     '_Cstart': 'z_cstart',
	     '_Swap': 'z_swap',
	     '_ClearFaults': 'z_clearfaults' }

r = fn

for p in ignore:
    if r.startswith(p):
        cocci.include_match(False)

for p in prefixes:
    if r.startswith(p):
        r = r[len(p):]

	while r.startswith('_'):
	    r = r[1:]

	if r:
	    if prefixes[p].endswith('_'):
	        r = prefixes[p] + r
	    else:
	        r = prefixes[p] + "_" + r
        else:
	    r = prefixes[p]

	break
else:
    dirstr = "/" + pos[0].file
    if dirstr.find("/ext/") > -1:
        cocci.include_match(False)

    while r.startswith('_'):
        r = r[1:]

    if r.isupper():
        r = 'Z_' + r
    else:
        r = 'z_' + r

coccinelle.z = r

@r_static_out depends on patch@
identifier r_staticfn.fn;
identifier r_static_rewrite.z;
@@

- fn
+ z

@r_staticdef_out depends on patch@
identifier r_staticfn.fn;
identifier r_static_rewrite.z;
type T;
@@

  T
- fn
+ z
  (...) {...}

@r_any_out depends on patch@
identifier r_anyfn.fn;
identifier r_any_rewrite.z;
@@

- fn
+ z

@r_anydef_out depends on patch@
identifier r_anyfn.fn;
identifier r_any_rewrite.z;
type T;
@@

  T
- fn
+ z
  (...) {...}

@r_anyproto_out depends on patch@
identifier r_anyfn.fn;
identifier r_any_rewrite.z;
type T;
@@

  T
- fn
+ z
  (...);

@script:python depends on report@
fn << r_anyfn.fn;
pos << r_anyfn.p;
z << r_any_rewrite.z;
@@
msg="WARNING: Reserved function name " + fn + ", should be " + z
coccilib.report.print_report(pos[0], msg)

@script:python depends on report@
fn << r_staticfn.fn;
pos << r_staticfn.p;
z << r_static_rewrite.z;
@@
msg="WARNING: Reserved static function name " + fn + ", should be " + z
coccilib.report.print_report(pos[0], msg)

@script:python depends on changes@
fn << r_anyfn.fn;
z << r_any_rewrite.z;
@@

print ("%s %s") % (fn, z)

@script:python depends on changes@
fn << r_staticfn.fn;
z << r_static_rewrite.z;
@@

print ("%s %s") % (fn, z)
