/// Find cases where ztest string comparisons macros can be used
// Copyright: (C) 2024 Nordic Semiconductor ASA
// SPDX-License-Identifier: Apache-2.0
// Confidence: LOW
// Options: --no-includes --include-headers

virtual patch
virtual context
virtual org
virtual report

// Comparing result of strcmp with 0

@@ expression E1,E2; @@
- zassert_equal(strcmp(E1, E2), 0);
+ zassert_str_equal(E1, E2);

@@ expression E1,E2,E3; @@
- zassert_equal(strcmp(E1, E2), 0, E3);
+ zassert_str_equal(E1, E2, E3);

@@ expression E1,E2; @@
- zassert_equal(0, strcmp(E1, E2));
+ zassert_str_equal(E1, E2);

@@ expression E1,E2,E3; @@
- zassert_equal(0, !strcmp(E1, E2), E3);
+ zassert_str_equal(E1, E2, E3);



// Using assert_true with !strcmp

@@ expression E1,E2; @@
- zassert_true(!strcmp(E1, E2));
+ zassert_str_equal(E1, E2);

@@ expression E1,E2,E3; @@
- zassert_true(!strcmp(E1, E2), E3);
+ zassert_str_equal(E1, E2, E3);


// using zassert_true with strcmp(E1, E2) == 0

@@expression E1,E2; @@
- zassert_true(strcmp(E1, E2) == 0);
+ zassert_str_equal(E1, E2);

@@expression E1,E2; @@
- zassert_true((strcmp(E1, E2) == 0));
+ zassert_str_equal(E1, E2);

@@expression E1,E2,E3; @@
- zassert_true(strcmp(E1, E2) == 0, E3);
+ zassert_str_equal(E1, E2, E3);

@@expression E1,E2,E3; @@
- zassert_true((strcmp(E1, E2) == 0), E3);
+ zassert_str_equal(E1, E2, E3);



// using zassert_true with 0 == strcmp(E1, E2)

@@expression E1,E2; @@
- zassert_true(0 == strcmp(E1, E2));
+ zassert_str_equal(E1, E2);

@@expression E1,E2; @@
- zassert_true((0 == strcmp(E1, E2)));
+ zassert_str_equal(E1, E2);

@@expression E1,E2,E3; @@
- zassert_true(0 == strcmp(E1, E2), E3);
+ zassert_str_equal(E1, E2, E3);

@@expression E1,E2,E3; @@
- zassert_true((0 == strcmp(E1, E2)), E3);
+ zassert_str_equal(E1, E2, E3);
