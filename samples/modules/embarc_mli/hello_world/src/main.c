/*
* Copyright 2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/
#include "mli_api.h"

#include <assert.h>
#include <stdio.h>

int main() {
    printf("************************************\n");
    int8_t data_in[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    mli_tensor in = { 0 };
    in.el_type = MLI_EL_FX_8;
    in.rank = 1;
    in.shape[0] = 8;
    in.mem_stride[0] = 1;
    in.data.capacity = sizeof(data_in);
    in.data.mem.void_p = data_in;

    int8_t data_out[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    mli_tensor out = { 0 };
    out.el_type = MLI_EL_FX_8;
    out.rank = 1;
    out.shape[0] = 8;
    out.mem_stride[0] = 1;
    out.data.capacity = sizeof(data_out);
    out.data.mem.void_p = data_out;

    mli_status status;
    status = mli_krn_eltwise_add_fx8(&in, &in, &out);
    assert(status == MLI_STATUS_OK);
    for (int i = 0; i < sizeof(data_out)/sizeof(data_out[0]); i++) {
        printf("%d ", ((int8_t*)(out.data.mem.void_p))[i]);
    }
    printf("\n");

    status = mli_krn_eltwise_sub_fx8(&in, &in, &out);
    assert(status == MLI_STATUS_OK);
    for (int i = 0; i < sizeof(data_out)/sizeof(data_out[0]); i++) {
        printf("%d ", ((int8_t*)(out.data.mem.void_p))[i]);
    }
    printf("\n");
    printf("************************************\n");
    return 0;
}
