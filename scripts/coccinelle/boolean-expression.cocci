/// Make statements evaluate Boolean expressions
///
// Confidence: Low
// Copyright (c) 2018 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual patch

@depends on !(file in "ext")@
expression *X;
statement S;
@@

- if (X)
+ if (X != NULL)
  S

@depends on !(file in "ext")@
expression *X;
statement S1, S2;
@@

- if (X) {
+ if (X != NULL) {
S1
} else {
S2
}

@depends on !(file in "ext")@
expression *X;
statement S;
@@

- if (X) {
+ if (X != NULL) {
S
}

@depends on !(file in "ext")@
typedef uint8_t;
typedef u8_t;
typedef uint16_t;
typedef u16_t;
typedef uint32_t;
typedef u32_t;
{uint8_t,u8_t,uint16_t,u16_t,uint32_t,u32_t,unsigned int,int,size_t,unsigned} I;
statement S;
@@

- if (!I)
+ if (I == 0)
S

@depends on !(file in "ext")@
expression *E;
@@

  E ==
- 0
+ NULL

@depends on !(file in "ext")@
expression *E;
@@

- 0
+ NULL
  == E

@depends on !(file in "ext")@
expression *E;
@@

  E !=
- 0
+ NULL

@depends on !(file in "ext")@
expression *E;
@@

- 0
+ NULL
  != E


@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (X && E)
+ if ((X != NULL) && E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (!X && E)
+ if ((X == NULL) && E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (E && X)
+ if (E && (X != NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (!X && E)
+ if (E && (X == NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
statement S;
@@

- if (!X)
+ if (X == NULL)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (X || E)
+ if ((X != NULL) || E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (!X || E)
+ if ((X == NULL) || E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (E || X)
+ if (E || (X != NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- if (!X || E)
+ if (E || (X == NULL))
  S

@depends on !(file in "ext")@
statement S;
expression E;
constant C;
@@

- if (E & C)
+ if ((E & C) != 0)
  S

@depends on !(file in "ext")@
statement S;
identifier I;
constant C;
@@

- if (!(I & C))
+ if ((I & C) == 0)
  S


@depends on !(file in "ext")@
statement S;
expression E;
constant C;
@@

- if (C & E)
+ if ((C & E) != 0)
  S

@depends on !(file in "ext")@
statement S;
identifier I;
constant C;
@@

- if (I & C)
+ if ((I & C) != 0)
  S

@depends on !(file in "ext")@
statement S;
identifier I;
expression E;
@@

- if (I & E)
+ if ((E & I) != 0)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (X && E)
+ while ((X != NULL) && E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (!X && E)
+ while ((X == NULL) && E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (E && X)
+ while (E && (X != NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (!X && E)
+ while (E && (X == NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
statement S;
@@

- while (X)
+ while (X != NULL)
  S

@depends on !(file in "ext")@
idexpression *X;
statement S;
@@

- while (!X)
+ while (X == NULL)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (X || E)
+ while ((X != NULL) || E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (!X || E)
+ while ((X == NULL) || E)
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (E || X)
+ while (E || (X != NULL))
  S

@depends on !(file in "ext")@
idexpression *X;
expression E;
statement S;
@@

- while (!X || E)
+ while (E || (X == NULL))
  S

@depends on !(file in "ext")@
statement S;
@@

- if (0)
+ if (false)
  S

@depends on !(file in "ext")@
statement S;
@@

- if (1)
+ if (true)
  S

@depends on !(file in "ext")@
statement S;
@@

- while (0)
+ while (false)
  S

@depends on !(file in "ext")@
statement S;
@@

- while (1)
+ while (true)
  S
