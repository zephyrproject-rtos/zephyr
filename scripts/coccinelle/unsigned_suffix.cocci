/// Find assignments to unsigned variables and add an 'U' to the value
// Copyright: (C) 2018 Intel Corporation
// Copyright: (C) 2018 Himanshu Jha
// Copyright: (C) 2018 Julia Lawall, Inria/LIP6
// SPDX-License-Identifier: Apache-2.0
// Confidence: High

virtual patch
virtual report

@r_unsigned@
typedef uint8_t, uint16_t, uint32_t, uint64_t, u8_t, u16_t, u32_t, u64_t;
{unsigned char, unsigned short, unsigned int, uint8_t, uint16_t, uint32_t, uint64_t, u8_t, u16_t, u32_t, u64_t} v;
constant C;
position p;
@@

(
  v = C@p
|
  v == C@p
|
  v != C@p
|
  v <= C@p
|
  v >= C@p
|
  v += C@p
|
  v -= C@p
|
  v * C@p
|
  v / C@p
|
  v *= C@p
|
  v /= C@p
)

@script:python r_rewrite@
C << r_unsigned.C;
z;
@@

if C.isdigit() != True:
    cocci.include_match(False)

coccinelle.z = C + "U"

@r_subst depends on patch@
{unsigned char, unsigned short, unsigned int, uint8_t, uint16_t, uint32_t, uint64_t, u8_t, u16_t, u32_t, u64_t} r_unsigned.v;
constant r_unsigned.C;
identifier r_rewrite.z;
@@

(
  v =
- C
+ z
|
  v ==
- C
+ z
|
  v !=
- C
+ z
|
  v <=
- C
+ z
|
  v >=
- C
+ z
|
  v +=
- C
+ z
|
  v -=
- C
+ z
|
  v +
- C
+ z
|
  v -
- C
+ z
|
  v +=
- C
+ z
|
  v -=
- C
+ z
|
- v * C
+ v * z
|
  v /
- C
+ z
|
  v *=
- C
+ z
|
  v /=
- C
+ z
)

@script: python depends on report@
p << r_unsigned.p;
@@

msg="WARNING: Unsigned 'U' suffix missing"
coccilib.report.print_report(p[0], msg)
