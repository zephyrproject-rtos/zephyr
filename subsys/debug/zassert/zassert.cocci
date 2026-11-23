// Copyright: (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
// SPDX-License-Identifier: Apache-2.0
// Options: --no-includes --include-headers

// A migrates existing __ASSERT and __ASSERT_NO_MSG to ZASSERT
// This script leaves the caller to manually add the ZASSERT_MODULE(..) to each TU that is migrated
// with the applicable module identifier.

virtual patch
virtual report

@@
@@
- #include <zephyr/sys/__assert.h>
+ #include <zephyr/sys/zassert.h>

@@
expression E;
expression list EL;
@@
- __ASSERT(E, EL)
+ ZASSERT(E, EL)

@@
expression E;
@@
- ZASSERT(E, "")
+ ZASSERT(E)

@@
expression E;
@@
- __ASSERT(E)
+ ZASSERT(E)

@@
expression E;
@@
- __ASSERT_NO_MSG(E)
+ ZASSERT(E)
