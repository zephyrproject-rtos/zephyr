// Copyright (c) 2017 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0//

@@
expression e1,e2,e3;
@@

- memset(e1,e2,e3);
+ (void)memset(e1,e2,e3);

