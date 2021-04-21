// Copyright (c) 2020 Intel Corporation
// SPDX-License-Identifer: Apache-2.0

// Uses a python database (a dict) to find where const struct device
// variable are being used in zephyr functions and, if it's being in place
// of a void*, it will print an ERROR for loosing the const qualifier.
// If it's being used on an unknown functions from an external module such
// as a HAL, it will print a WARNING in order to check if the const qualifier
// is not lost.

virtual report

////////////////////
// Initialization //
////////////////////

@initialize:python
  depends on report
@
@@
import pickle

def check_and_report(F, f, D, nb_args, p):
    if f in f_void and int(nb_args) in f_void[f]:
        msg = "ERROR: in {} calling {} param with {}, \
loosing const qualifier, please wrap".format(F, f, D)
        coccilib.report.print_report(p[0], msg)
    elif f not in f_void and f not in f_other and not f.isupper():
        msg = "WARNING: in {} calling {} param with {}, \
check if const qualifier is not lost".format(F, f, D)
        coccilib.report.print_report(p[0], msg)

// Loading function data base
with open("function_names.pickle", "rb") as f:
    data = pickle.load(f)
f_void = data["f_void"]
f_other = data["f_other"]


///////////
// Rules //
///////////

// Find usage of a device instance
@r_find_dev_usage
  depends on report
@
local idexpression struct device *D;
expression list[nb_args] args;
identifier f;
position p;
@@
  f(args, D@p, ...)


@script:python
  depends on r_find_dev_usage
@
f << r_find_dev_usage.f;
D << r_find_dev_usage.D;
nb_args << r_find_dev_usage.nb_args;
p << r_find_dev_usage.p;
@@
check_and_report(p[0].current_element, f, D, nb_args, p)
