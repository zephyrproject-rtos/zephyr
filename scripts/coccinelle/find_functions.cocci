// Copyright (c) 2020 Intel Corporation
// SPDX-License-Identifer: Apache-2.0

// In patch mode, patch all device instance to const (if not already).

// In report mode:
// Generate a q&d python database (a dict actually) of all the
// declared zephyr functions. It will store each function name in 2
// separate dicts: one storing all function having 1+ void* parameter
// and one for all the other functions. It will store the positions
// of the void* parameter in the first dict, and the actual number of
// parameters in the second.
// Then find_dev_usage.cocci can be used to verify if device instance
// are not loosing their const qualifier.

virtual patch
virtual report


////////////////////
// Initialization //
////////////////////

@initialize:python
  depends on report
@
@@
import pickle
f_void = {}
f_other = {}

// Insert a function into right dict depending on parameters
def insert_function(f, params):
    void_pos = []
    i = 0
    for prm in params:
        if prm.startswith("void *"):
            void_pos.append(i)
        i += 1
    if len(void_pos) != 0:
        f_void[f] = void_pos
    else:
        f_other[f] = i + 1


///////////
// Rules //
///////////

// Switch device instance to constant.
@r_const_dev
  depends on patch
  disable optional_qualifier
@
@@
-struct device *
+const struct device *


// Find function declarations
@r_find_func_declare
  depends on report
@
identifier f;
type ret_type;
parameter list[nb_params] params;
@@
  ret_type f(params);

// Insert function declaration
@script:python
  depends on report
@
f << r_find_func_declare.f;
params << r_find_func_declare.params;
@@
insert_function(f, params)


// Find function implementations and inlines
// (maybe it should focus on static only?
// but then first rule should not match statics.)
@r_find_func_impl_inlines
  depends on report
@
identifier f;
type ret_type;
parameter list[nb_params] params;
@@
(
  ret_type f(params)
  {
    ...
  }
|
  static inline ret_type f(params)
  {
    ...
  }
)

// Insert function implentations and inlines
@script:python
  depends on report
@
f << r_find_func_impl_inlines.f;
params << r_find_func_impl_inlines.params;
@@
insert_function(f, params)


//////////////////
// Finalization //
//////////////////

@finalize:python
  depends on report
@
@@
with open("function_names.pickle", "wb") as f:
    data = {}
    data['f_void'] = f_void
    data['f_other'] = f_other
    pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)
