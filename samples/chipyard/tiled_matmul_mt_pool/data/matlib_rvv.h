#pragma once
#ifndef TINYMPC_MATLIB_RVV_H
#define TINYMPC_MATLIB_RVV_H

#include <cstdio>
#include <climits>
#include <cmath>

#include "riscv_vector.h"

extern "C"
{

// RVV-specific function declarations
float maxcoeff_rvv(float *a, int n, int m);
float mincoeff_rvv(float *a, int n, int m);
float matnorm_rvv(float *a, int n, int m);
void matneg_rvv(float *a, float *b, int n, int m);
void cwiseabs_rvv(float *a, float *b, int n, int m);
void cwisemin_rvv(float *a, float *b, float *c, int n, int m);
void cwisemax_rvv(float *a, float *b, float *c, int n, int m);
void cwisemul_rvv(float *a, float *b, float *c, int n, int m);
void matmul_rvv(float *a, float *b, float *c, int n, int m, int o, int tile_size);
void matmul_rvvt(float *a, float *b, float *c, int i, int j, int k, int n, int m, int o, int tile_size);
void matvec_rvv(float *a, float *b, float *c, int n, int m);
void matvec_transpose_rvv(float *a, float *b, float *c, int n, int m);
void matmulf_rvv(float *a, float *b, float f, int n, int m);
void matsub_rvv(float *a, float *b, float *c, int n, int m);
void matadd_rvv(float *a, float *b, float *c, int n, int m);
void transpose_rvv(float *a, float *b, int n, int m);
void matcopy_rvv(const float *a, float *b, int n, int m);
void matset_rvv(float *a, float f, int n, int m);
void matsetv_rvv(float *a, float *f, int n, int m);

#define maxcoeff maxcoeff_rvv
#define mincoeff mincoeff_rvv
#define matnorm matnorm_rvv
#define matneg matneg_rvv
#define cwiseabs cwiseabs_rvv
#define cwisemin cwisemin_rvv
#define cwisemax cwisemax_rvv
#define cwisemul cwisemul_rvv
#define matmul matmul_rvv
#define matvec matvec_rvv
#define matvec_transpose matvec_transpose_rvv
#define matmulf matmulf_rvv
#define matsub matsub_rvv
#define matadd matadd_rvv
#define transpose transpose_rvv
#define matcopy matcopy_rvv
#define matset matset_rvv
#define matsetv matsetv_rvv


// matrix maximum coefficient
inline float maxcoeff_rvv(float *ptr_a, int n, int m) {
    float max = std::numeric_limits<float>::min();
    vfloat32m1_t vec_max = __riscv_vfmv_s_f_f32m1(max, 1);
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vec_max = __riscv_vfredmax_vs_f32_f32(vec_a, vec_max, vl);
    }
    max = __riscv_vfmv_f_s_f32m1_f32(vec_max);
    return max;
}

// matrix min coefficient
inline float mincoeff_rvv(float *ptr_a, int n, int m) {
    float min = std::numeric_limits<float>::max();
    vfloat32m1_t vec_min = __riscv_vfmv_s_f_f32m1(min, 1);
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vec_min = __riscv_vfredmin_vs_f32_f32(vec_a, vec_min, vl);
    }
    min = __riscv_vfmv_f_s_f32m1_f32(vec_min);
    return min;
}

// matrix unary negative
inline void matneg_rvv(float *ptr_a, float *ptr_b, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vfneg_v_f32(vec_a, vl);
        __riscv_vse32_v_f32(ptr_b + l, vec_b, vl);
    }
}

// matrix l2 norm
inline float matnorm_rvv(float *ptr_a, int n, int m) {
    int k = m * n, l = 0;
    size_t vlmax = __riscv_vsetvlmax_e32();
    vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);
    vfloat32_t vec_s = __riscv_vfmv_v_f_f32(0, vlmax);
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vec_s = __riscv_vfmacc_vv_f32(vec_s, vec_a, vec_a, vl);
    }
    vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32_f32(vec_s, vec_zero, vlmax);
    float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
    return sqrt(sum);
}

// matrix coefficient-wise abs
inline void cwiseabs_rvv(float *ptr_a, float *ptr_b, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vfabs_v_f32(vec_a, vl);
        __riscv_vse32_v_f32(ptr_b + l, vec_b, vl);
    }
}

// matrix coefficient-wise min
inline void cwisemin_rvv(float *ptr_a, float *ptr_b, float *ptr_c, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b + l, vl);
        vfloat32_t vec_c = __riscv_vfmin_vv_f32(vec_a, vec_b, vl);
        __riscv_vse32_v_f32(ptr_c + l, vec_c, vl);
    }
}

// matrix coefficient-wise multiplication
inline void cwisemul_rvv(float *ptr_a, float *ptr_b, float *ptr_c, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b + l, vl);
        vfloat32_t vec_c = __riscv_vfmul_vv_f32(vec_a, vec_b, vl);
        __riscv_vse32_v_f32(ptr_c + l, vec_c, vl);
    }
}

// matrix coefficient-wise max
inline void cwisemax_rvv(float *ptr_a, float *ptr_b, float *ptr_c, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b + l, vl);
        vfloat32_t vec_c = __riscv_vfmax_vv_f32(vec_a, vec_b, vl);
        __riscv_vse32_v_f32(ptr_c + l, vec_c, vl);
    }
}

// matrix multiplication, note B is not [o][m]
// A[n][o], B[m][o] --> C[n][m];
inline void matmul_rvv(float *a, float *b, float *c, int n, int m, int o, int tile_size = -1) {
    if (tile_size == -1) {
        size_t vlmax = __riscv_vsetvlmax_e32();
        vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                float *ptr_a = a + i * o; // row major
                float *ptr_b = b + j * o; // column major
                int k = o;
                vfloat32_t vec_s = __riscv_vfmv_v_f_f32(0, vlmax);
                for (size_t vl; k > 0; k -= vl, ptr_a += vl, ptr_b += vl) {
                    vl = __riscv_vsetvl_e32(k);
                    vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a, vl);
                    vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b, vl);
                    vec_s = __riscv_vfmacc_vv_f32(vec_s, vec_a, vec_b, vl);
                }
                vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32_f32(vec_s, vec_zero, vlmax);
                float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
                c[i * m + j] = sum;
            }
        }
    } else {
        // matset_rvv(c, 0.0f, n, m);
        for (int i = 0; i < n; i += tile_size) {
            for (int j = 0; j < m; j += tile_size) {
                for (int k = 0; k < o; k += tile_size) {
                    // printf("i: %d j: %d k: %d n: %d m: %d o: %d\n", i, j, k, n, m, o);
                    matmul_rvvt(a, b, c, i, j, k, n, m, o, tile_size);
                }
            }
        }
    }
}

inline void matmul_rvvt(float *a, float *b, float *c, int i, int j, int k, int n, int m, int o, int tile_size) {
    float *A = a + i * o + k;
    float *B = b + j * o + k;
    float *C = c + i * m + j;
    int N = i + tile_size <= n ? tile_size : n % tile_size;
    int M = j + tile_size <= m ? tile_size : m % tile_size;
    int O = k + tile_size <= o ? tile_size : o % tile_size;
    // printf("A: %d B: %d C: %d N: %d M: %d O: %d\n", A, B, C, N, M, O);
    size_t vlmax = __riscv_vsetvlmax_e32();
    vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);
    for (int I = 0; I < N; ++I) {
        for (int J = 0; J < M; ++J) {
            float *ptr_a = A + I * o; // row major
            float *ptr_b = B + J * o; // column major
            int K = O;
            vfloat32_t vec_s = __riscv_vfmv_v_f_f32(0, vlmax);
            for (size_t vl; K > 0; K -= vl, ptr_a += vl, ptr_b += vl) {
                vl = __riscv_vsetvl_e32(K);
                // printf("I: %d J: %d K: %d N: %d M: %d O: %d ptr_a: %x ptr_b: %x vl: %d\n", I, J, K, N, M, O, ptr_a, ptr_b, vl);
                vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a, vl);
                vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b, vl);
                vec_s = __riscv_vfmacc_vv_f32(vec_s, vec_a, vec_b, vl);
            }
            vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32_f32(vec_s, vec_zero, vlmax);
            float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
            C[I * m + J] = k == 0 ? sum : C[I * m + J] + sum;
        }
    }
}

/*  a is row major
 *        j
 *    1 2 3 4     9
 *  i 5 6 7 8  *  6
 *    9 8 7 6     5 j
 *                4
 */
inline void matvec_rvv(float *a, float *b, float *c, int n, int m) {
    size_t vl = 0;
    size_t vlmax = __riscv_vsetvlmax_e32();
    vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);
    int k = n;
    for (int I = 0; k > 0; k -= vl, I += vl) {
        vl = __riscv_vsetvl_e32(k);
        float *ptr_a = a + I * m; // row major
        float *ptr_b = b; // column major
        vfloat32_t vec_c = __riscv_vfmv_v_f_f32(0, vlmax);
        for (int J = 0; J < m; J++) {
            vfloat32_t vec_a = __riscv_vlse32_v_f32(ptr_a + J, m * sizeof(float), vl);
            vec_c = __riscv_vfmacc_vf_f32(vec_c, *(ptr_b + J), vec_a, vl);
        }
        __riscv_vse32_v_f32(c + I, vec_c, vl);
    }
}

/*  a is col major
 *      j         i
 *  9 6 5 4  *  1 5 9
 *              2 6 8
 *              3 7 7 j
 *              4 8 6
 */
inline void matvec_transpose_rvv(float *a, float *b, float *c, int n, int m) {
    size_t vlmax = __riscv_vsetvlmax_e32();
    vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);
    int k = m;
    size_t vl = 0;
    for (int I = 0; k > 0; k -= vl, I += vl) {
        vl = __riscv_vsetvl_e32(k);
        float *ptr_a = a + I; // col major
        float *ptr_b = b; // row major
        vfloat32_t vec_c = __riscv_vfmv_v_f_f32(0, vlmax);
        for (int J = 0; J < n; J++) {
            vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + J * m, vl);
            vec_c = __riscv_vfmacc_vf_f32(vec_c, *(ptr_b + J), vec_a, vl);
        }
        __riscv_vse32_v_f32(c + I, vec_c, vl);
    }
}

// matrix scalar multiplication
inline void matmulf_rvv(float *ptr_a, float *ptr_b, float f, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vfmul_vf_f32(vec_a, f, vl);
        __riscv_vse32_v_f32(ptr_b + l, vec_b, vl);
    }
}

// matrix subtraction
inline void matsub_rvv(float *ptr_a, float *ptr_b, float *ptr_c, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b + l, vl);
        vfloat32_t vec_c = __riscv_vfsub_vv_f32(vec_a, vec_b, vl);
        __riscv_vse32_v_f32(ptr_c + l, vec_c, vl);
    }
}

// matrix addition
inline void matadd_rvv(float *ptr_a, float *ptr_b, float *ptr_c, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        vfloat32_t vec_b = __riscv_vle32_v_f32(ptr_b + l, vl);
        vfloat32_t vec_c = __riscv_vfadd_vv_f32(vec_a, vec_b, vl);
        __riscv_vse32_v_f32(ptr_c + l, vec_c, vl);
    }
}

// matrix transpose
inline void transpose_rvv(float *a, float *b, int n, int m) {
    for (int j = 0; j < m; ++j) {
        float *ptr_a = a + j;
        float *ptr_b = b + j * n;
        int k = n;
        int l = 0;
        for (size_t vl; k > 0; k -= vl, l += vl, ptr_a = a + l * m + j, ptr_b += vl) {
            vl = __riscv_vsetvl_e32(k);
            vfloat32_t vec_a = __riscv_vlse32_v_f32(ptr_a, sizeof(float) * m, vl);
            __riscv_vse32(ptr_b, vec_a, vl);
        }
    }
};

// matrix copy
inline void matcopy_rvv(const float *ptr_a, float *ptr_b, int n, int m) {
    int k = n * m, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vle32_v_f32(ptr_a + l, vl);
        __riscv_vse32_v_f32(ptr_b + l, vec_a, vl);
    }
}

inline void matset_rvv(float *ptr_a, float f, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_a = __riscv_vfmv_v_f_f32(f, vl);
        __riscv_vse32_v_f32(ptr_a + l, vec_a, vl);
    }
}

inline void matsetv_rvv(float *ptr_a, float *f, int n, int m) {
    int k = m * n, l = 0;
    for (size_t vl; k > 0; k -= vl, l += vl) {
        vl = __riscv_vsetvl_e32(k);
        vfloat32_t vec_f = __riscv_vle32_v_f32(f + l, vl);;
        __riscv_vse32_v_f32(ptr_a + l, vec_f, vl);
    }
}

}
#endif //TINYMPC_MATLIB_RVV_H
