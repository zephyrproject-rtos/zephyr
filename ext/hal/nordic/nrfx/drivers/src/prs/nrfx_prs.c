/*
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx.h>

#if NRFX_CHECK(NRFX_PRS_ENABLED)
#include "nrfx_prs.h"

#define NRFX_LOG_MODULE PRS
#include <nrfx_log.h>

#define LOG_FUNCTION_EXIT(level, ret_code)            \
    NRFX_LOG_##level("Function: %s, error code: %s.", \
        __func__,                                     \
        NRFX_LOG_ERROR_STRING_GET(ret_code))


typedef struct {
    nrfx_irq_handler_t handler;
    bool               acquired;
} prs_box_t;

#define PRS_BOX_DEFINE(n)                                                    \
    static prs_box_t m_prs_box_##n = { .handler = NULL, .acquired = false }; \
    void nrfx_prs_box_##n##_irq_handler(void)                                \
    {                                                                        \
        NRFX_ASSERT(m_prs_box_##n.handler);                                  \
        m_prs_box_##n.handler();                                             \
    }

#if defined(NRFX_PRS_BOX_0_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_0_ENABLED)
PRS_BOX_DEFINE(0)
#endif
#if defined(NRFX_PRS_BOX_1_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_1_ENABLED)
PRS_BOX_DEFINE(1)
#endif
#if defined(NRFX_PRS_BOX_2_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_2_ENABLED)
PRS_BOX_DEFINE(2)
#endif
#if defined(NRFX_PRS_BOX_3_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_3_ENABLED)
PRS_BOX_DEFINE(3)
#endif
#if defined(NRFX_PRS_BOX_4_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_4_ENABLED)
PRS_BOX_DEFINE(4)
#endif


static prs_box_t * prs_box_get(void const * p_base_addr)
{
#if !defined(IS_PRS_BOX)
#define IS_PRS_BOX(n, p_base_addr)  ((p_base_addr) == NRFX_PRS_BOX_##n##_ADDR)
#endif

#if defined(NRFX_PRS_BOX_0_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_0_ENABLED)
    if (IS_PRS_BOX(0, p_base_addr)) { return &m_prs_box_0; }
    else
#endif
#if defined(NRFX_PRS_BOX_1_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_1_ENABLED)
    if (IS_PRS_BOX(1, p_base_addr)) { return &m_prs_box_1; }
    else
#endif
#if defined(NRFX_PRS_BOX_2_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_2_ENABLED)
    if (IS_PRS_BOX(2, p_base_addr)) { return &m_prs_box_2; }
    else
#endif
#if defined(NRFX_PRS_BOX_3_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_3_ENABLED)
    if (IS_PRS_BOX(3, p_base_addr)) { return &m_prs_box_3; }
    else
#endif
#if defined(NRFX_PRS_BOX_4_ADDR) && NRFX_CHECK(NRFX_PRS_BOX_4_ENABLED)
    if (IS_PRS_BOX(4, p_base_addr)) { return &m_prs_box_4; }
    else
#endif
    {
        return NULL;
    }
}

nrfx_err_t nrfx_prs_acquire(void       const * p_base_addr,
                            nrfx_irq_handler_t irq_handler)
{
    NRFX_ASSERT(p_base_addr);

    nrfx_err_t ret_code;

    prs_box_t * p_box = prs_box_get(p_base_addr);
    if (p_box != NULL)
    {
        bool busy = false;

        NRFX_CRITICAL_SECTION_ENTER();
        if (p_box->acquired)
        {
            busy = true;
        }
        else
        {
            p_box->handler  = irq_handler;
            p_box->acquired = true;
        }
        NRFX_CRITICAL_SECTION_EXIT();

        if (busy)
        {
            ret_code = NRFX_ERROR_BUSY;
            LOG_FUNCTION_EXIT(WARNING, ret_code);
            return ret_code;
        }
    }

    ret_code = NRFX_SUCCESS;
    LOG_FUNCTION_EXIT(INFO, ret_code);
    return ret_code;
}

void nrfx_prs_release(void const * p_base_addr)
{
    NRFX_ASSERT(p_base_addr);

    prs_box_t * p_box = prs_box_get(p_base_addr);
    if (p_box != NULL)
    {
        p_box->handler  = NULL;
        p_box->acquired = false;
    }
}


#endif // NRFX_CHECK(NRFX_PRS_ENABLED)
