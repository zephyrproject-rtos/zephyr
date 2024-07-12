/* tls_threaded.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#define USE_CERT_BUFFERS_256
#include <wolfssl/certs_test.h>
#include <wolfssl/test.h>

#ifdef WOLFSSL_ZEPHYR
#define printf   printk
#endif

/* wolfSSL PSA Crypto API integration with ECDH/ECDSA currently requires
 * use of wolfSSL Public Key (PK) callbacks.
 *
 * PSA Crypto API integration for this sample was tested on a
 * Nordic nRF5340dk.
 */
#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    #include <wolfssl/wolfcrypt/port/psa/psa.h>
#endif

#define BUFFER_SIZE           2048
#define STATIC_MEM_SIZE       (192*1024)

#ifdef WOLFSSL_STATIC_MEMORY
    static WOLFSSL_HEAP_HINT* HEAP_HINT_SERVER;
    static WOLFSSL_HEAP_HINT* HEAP_HINT_CLIENT;

    static byte gMemoryServer[STATIC_MEM_SIZE];
    static byte gMemoryClient[STATIC_MEM_SIZE];
#else
    #define HEAP_HINT_SERVER NULL
    #define HEAP_HINT_CLIENT NULL
#endif /* WOLFSSL_STATIC_MEMORY */

/* Buffer to hold data for client to read. */
unsigned char client_buffer[BUFFER_SIZE];
int client_buffer_sz = 0;
wolfSSL_Mutex client_mutex;

/* Buffer to hold data for server to read. */
unsigned char server_buffer[BUFFER_SIZE];
int server_buffer_sz = 0;
wolfSSL_Mutex server_mutex;

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
static struct psa_ssl_ctx server_psa_ctx;
static struct psa_ssl_ctx client_psa_ctx;
/* psa_key_id_t representing server key loaded into PSA Crypto API */
static psa_key_id_t ecc_key_id;
#endif

/* Application data to send. */
static const char msgHTTPGet[] = "GET /index.html HTTP/1.0\r\n\r\n";
static const char msgHTTPIndex[] =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    "<head>\n"
    "<title>Welcome to wolfSSL!</title>\n"
    "</head>\n"
    "<body>\n"
    "<p>wolfSSL has successfully performed handshake!</p>\n"
    "</body>\n"
    "</html>\n";

#ifdef HAVE_FIPS
static void myFipsCb(int ok, int err, const char* hash)
{
    printf("in my Fips callback, ok = %d, err = %d\n", ok, err);
    printf("message = %s\n", wc_GetErrorString(err));
    printf("hash = %s\n", hash);

    if (err == IN_CORE_FIPS_E) {
        printf("In core integrity hash check failure, copy above hash\n");
        printf("into verifyCore[] in fips_test.c and rebuild\n");
    }
}
#endif

/* wolfSSL client wants to read data from the server. */
static int recv_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    wc_LockMutex(&client_mutex);
    if (client_buffer_sz > 0) {
        /* Take as many bytes is available or requested from buffer. */
        if (sz > client_buffer_sz)
            sz = client_buffer_sz;
        XMEMCPY(buff, client_buffer, sz);
        if (sz < client_buffer_sz) {
            XMEMMOVE(client_buffer, client_buffer + sz, client_buffer_sz - sz);
        }
        client_buffer_sz -= sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_READ;
    wc_UnLockMutex(&client_mutex);

    return sz;
}

/* wolfSSL client wants to write data to the server. */
static int send_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    wc_LockMutex(&server_mutex);
    if (server_buffer_sz < BUFFER_SIZE)
    {
        /* Put in as many bytes requested or will fit in buffer. */
        if (sz > BUFFER_SIZE - server_buffer_sz)
            sz = BUFFER_SIZE - server_buffer_sz;
        XMEMCPY(server_buffer + server_buffer_sz, buff, sz);
        server_buffer_sz += sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_WRITE;
    wc_UnLockMutex(&server_mutex);

    return sz;
}

/* wolfSSL server wants to read data from the client. */
static int recv_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    wc_LockMutex(&server_mutex);
    if (server_buffer_sz > 0) {
        /* Take as many bytes is available or requested from buffer. */
        if (sz > server_buffer_sz)
            sz = server_buffer_sz;
        XMEMCPY(buff, server_buffer, sz);
        if (sz < server_buffer_sz) {
            XMEMMOVE(server_buffer, server_buffer + sz, server_buffer_sz - sz);
        }
        server_buffer_sz -= sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_READ;
    wc_UnLockMutex(&server_mutex);

    return sz;
}

/* wolfSSL server wants to write data to the client. */
static int send_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    wc_LockMutex(&client_mutex);
    if (client_buffer_sz < BUFFER_SIZE)
    {
        /* Put in as many bytes requested or will fit in buffer. */
        if (sz > BUFFER_SIZE - client_buffer_sz)
            sz = BUFFER_SIZE - client_buffer_sz;
        XMEMCPY(client_buffer + client_buffer_sz, buff, sz);
        client_buffer_sz += sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_WRITE;
    wc_UnLockMutex(&client_mutex);

    return sz;
}

/* DO NOT use this in production. You should implement a way
 * to get the current date. */
static int verifyIgnoreDateError(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    if (store->error == ASN_BEFORE_DATE_E)
        return 1; /* override error */
    else
        return preverify;
}

/* Create a new wolfSSL client with a server CA certificate. */
static int wolfssl_client_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((client_ctx = wolfSSL_CTX_new_ex(wolfTLSv1_3_client_method(),
                                                   HEAP_HINT_CLIENT)) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
         if (wolfSSL_CTX_load_verify_buffer_ex(client_ctx, ca_ecc_cert_der_256,
                sizeof_ca_ecc_cert_der_256, WOLFSSL_FILETYPE_ASN1, 0,
                /* DO NOT use this in production. You should
                 * implement a way to get the current date. */
                WOLFSSL_LOAD_FLAG_DATE_ERR_OKAY) !=
                WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load CA certificate\n");
            ret = -1;
        }
    }

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    if (ret == 0) {
        if (wolfSSL_CTX_psa_enable(client_ctx) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to enable PSA Crypto API for WOLFSSL_CTX\n");
            ret = -1;
        }
    }
#endif

    if (ret == 0) {
        /* Register callbacks */
        wolfSSL_SetIORecv(client_ctx, recv_client);
        wolfSSL_SetIOSend(client_ctx, send_client);

        /* Create a WOLFSSL object */
        if ((client_ssl = wolfSSL_new(client_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0)
        wolfSSL_set_verify(client_ssl,
            WOLFSSL_VERIFY_PEER|WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT,
            verifyIgnoreDateError);

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    if (ret == 0) {
        XMEMSET(&client_psa_ctx, 0, sizeof(client_psa_ctx));
        if (wolfSSL_set_psa_ctx(client_ssl, &client_psa_ctx) != WOLFSSL_SUCCESS) {
            printf("ERROR: wolfSSL_set_psa_ctx() failed\n");
            ret = -1;
        }
    }
#endif

    if (ret == 0) {
        /* make wolfSSL object nonblocking */
        wolfSSL_set_using_nonblock(client_ssl, 1);

        /* Return newly created wolfSSL context and object */
        *ctx = client_ctx;
        *ssl = client_ssl;
    }
    else {
        if (client_ssl != NULL) {
#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
            wolfSSL_free_psa_ctx(&client_psa_ctx);
#endif
            wolfSSL_free(client_ssl);
        }
        if (client_ctx != NULL)
            wolfSSL_CTX_free(client_ctx);
    }

    return ret;
}

/* Client connecting to server using TLS */
static int wolfssl_client_connect(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        if (!wolfSSL_want_read(ssl) && !wolfSSL_want_write(ssl))
            ret = -1;
    }

    return ret;
}

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)

/* ./certs/ecc-key.pem */
static const unsigned char ecc_key_256[] =
{
    0x45, 0xB6, 0x69, 0x02, 0x73, 0x9C, 0x6C, 0x85, 0xA1, 0x38,
    0x5B, 0x72, 0xE8, 0xE8, 0xC7, 0xAC, 0xC4, 0x03, 0x8D, 0x53,
    0x35, 0x04, 0xFA, 0x6C, 0x28, 0xDC, 0x34, 0x8D, 0xE1, 0xA8,
    0x09, 0x8C
};

/* Provision server private key using PSA Crypto API.
 *
 * key_id - resulting psa_key_id_t
 *
 * Returns - 0 on success, negative on error
 */
static int psa_private_key_provisioning(psa_key_id_t *key_id)
{
    psa_key_attributes_t key_attr = { 0 };
    psa_key_type_t key_type;
    psa_status_t status;

    key_type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);

    psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
    psa_set_key_algorithm(&key_attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&key_attr, key_type);
    psa_set_key_bits(&key_attr, 256);

    status = psa_import_key(&key_attr, ecc_key_256,
                            sizeof(ecc_key_256), key_id);

    if (status != PSA_SUCCESS) {
        printf("ERROR: provisioning of private key failed: [%d] \n", status);
        return -1;
    }

    return 0;
}

#endif /* WOLFSSL_HAVE_PSA & HAVE_PK_CALLBACKS */

/* Create a new wolfSSL server with a certificate for authentication. */
static int wolfssl_server_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    /* Provision ECC private key with PSA Crypto API */
    if (psa_private_key_provisioning(&ecc_key_id) != 0) {
        printf("ERROR: failed to provision PSA private key\n");
        ret = -1;
    }

    if (ret == 0) {
        XMEMSET(&server_psa_ctx, 0, sizeof(server_psa_ctx));
        wolfSSL_psa_set_private_key_id(&server_psa_ctx, ecc_key_id);
    }
#endif

    if (ret == 0) {
        /* Create and initialize WOLFSSL_CTX */
        if ((server_ctx = wolfSSL_CTX_new_ex(wolfTLSv1_3_server_method(),
                                             HEAP_HINT_SERVER)) == NULL) {
            printf("ERROR: failed to create WOLFSSL_CTX\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_certificate_buffer(server_ctx,
                serv_ecc_der_256, sizeof_serv_ecc_der_256,
                WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server certificate\n");
            ret = -1;
        }
    }

#if !defined(WOLFSSL_HAVE_PSA) || \
    (defined(WOLFSSL_HAVE_PSA) && !defined(HAVE_PK_CALLBACKS))
    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_PrivateKey_buffer(server_ctx,
                ecc_key_der_256, sizeof_ecc_key_der_256,
                WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server key\n");
            ret = -1;
        }
    }
#else
    if (ret == 0) {
        if (wolfSSL_CTX_psa_enable(server_ctx) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to enable PSA\n");
            ret = -1;
        }
    }
#endif /* WOLFSSL_HAVE_PSA */

    if (ret == 0) {
        /* Register callbacks */
        wolfSSL_SetIORecv(server_ctx, recv_server);
        wolfSSL_SetIOSend(server_ctx, send_server);

        /* Create a WOLFSSL object */
        if ((server_ssl = wolfSSL_new(server_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0)
        wolfSSL_set_verify(server_ssl, WOLFSSL_VERIFY_PEER,
            verifyIgnoreDateError);

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    if (ret == 0) {
        if (wolfSSL_set_psa_ctx(server_ssl, &server_psa_ctx)
                != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to enable PSA in WOLFSSL struct\n");
            ret = -1;
        }
    }
#endif

    if (ret == 0) {
        /* make wolfSSL object nonblocking */
        wolfSSL_set_using_nonblock(server_ssl, 1);

        /* Return newly created wolfSSL context and object */
        *ctx = server_ctx;
        *ssl = server_ssl;
    }
    else {
        if (server_ssl != NULL) {
#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
            wolfSSL_free_psa_ctx(&server_psa_ctx);
#endif
            wolfSSL_free(server_ssl);
        }
        if (server_ctx != NULL)
            wolfSSL_CTX_free(server_ctx);
    }

    return ret;
}

/* Server accepting a client using TLS */
static int wolfssl_server_accept(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
        if (!wolfSSL_want_read(ssl) && !wolfSSL_want_write(ssl))
            ret = -1;
    }

    return ret;
}


/* Send application data. */
static int wolfssl_send(WOLFSSL* ssl, const char* msg)
{
    int ret = 0;
    int len;

    printf("Sending:\n%s\n", msg);
    len = wolfSSL_write(ssl, msg, XSTRLEN(msg));
    if (len < 0)
        ret = len;
    else if (len != XSTRLEN(msg))
        ret = -1;

    return ret;
}

/* Receive application data. */
static int wolfssl_recv(WOLFSSL* ssl)
{
    int ret;
    byte reply[256];

    ret = wolfSSL_read(ssl, reply, sizeof(reply)-1);
    if (ret > 0) {
        reply[ret] = '\0';
        printf("Received:\n%s\n", reply);
        ret = 1;
    }
    else if (wolfSSL_want_read(ssl) || wolfSSL_want_write(ssl))
        ret = 0;

    return ret;
}


/* Free the WOLFSSL object and context. */
static void wolfssl_free(WOLFSSL_CTX* ctx, WOLFSSL* ssl)
{
    if (ssl != NULL)
        wolfSSL_free(ssl);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
}


/* Display the static memory usage. */
static void wolfssl_memstats(WOLFSSL* ssl)
{
#ifdef WOLFSSL_STATIC_MEMORY
    WOLFSSL_MEM_CONN_STATS ssl_stats;

    XMEMSET(&ssl_stats, 0 , sizeof(ssl_stats));

    if (wolfSSL_is_static_memory(ssl, &ssl_stats) != 1)
        printf("static memory was not used with ssl");
    else {
        printf("*** This is memory state before wolfSSL_free is called\n");
        printf("peak connection memory = %d\n", ssl_stats.peakMem);
        printf("current memory in use  = %d\n", ssl_stats.curMem);
        printf("peak connection allocs = %d\n", ssl_stats.peakAlloc);
        printf("current connection allocs = %d\n",ssl_stats.curAlloc);
        printf("total connection allocs   = %d\n",ssl_stats.totalAlloc);
        printf("total connection frees    = %d\n\n", ssl_stats.totalFr);
    }
#else
    (void)ssl;
#endif
}

/* Thread to do the server operations. */
void server_thread(void* arg1)
{
    int ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;

#ifdef WOLFSSL_STATIC_MEMORY
    if (wc_LoadStaticMemory(&HEAP_HINT_SERVER, gMemoryServer,
                               sizeof(gMemoryServer),
                               WOLFMEM_GENERAL | WOLFMEM_TRACK_STATS, 1) != 0) {
        printf("unable to load static memory");
        ret = -1;
    }

    if (ret == 0)
#endif
        ret = wolfssl_server_new(&server_ctx, &server_ssl);

    while (ret == 0) {
        ret = wolfssl_server_accept(server_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(server_ssl))
            break;
    }

    /* Receive HTTP request */
    while (ret == 0) {
        ret = wolfssl_recv(server_ssl);
    }
    if (ret == 1)
        ret = 0;
    /* Send HTTP response */
    if (ret == 0)
        ret = wolfssl_send(server_ssl, msgHTTPIndex);

    printf("Server Return: %d\n", ret);
    printf("Server Error: %d\n", wolfSSL_get_error(server_ssl, ret));

#ifdef WOLFSSL_STATIC_MEMORY
    printf("Server Memory Stats\n");
#endif
    wolfssl_memstats(server_ssl);

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    if (server_ssl != NULL) {
        wolfSSL_free_psa_ctx(&server_psa_ctx);
    }
#endif
    wolfssl_free(server_ctx, server_ssl);
}

int main()
{
    int ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;
    THREAD_TYPE  serverThread;

    /* set dummy wallclock time for cert validation without NTP/etc */
    struct timespec utctime;
    utctime.tv_sec = 1658510212; /* Friday, July 22, 2022 5:16:52 PM GMT */
    utctime.tv_nsec = 0;
    clock_settime(CLOCK_REALTIME, &utctime);

#ifdef HAVE_FIPS
    wolfCrypt_SetCb_fips(myFipsCb);
#endif
    wolfSSL_Init();
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    wc_InitMutex(&client_mutex);
    wc_InitMutex(&server_mutex);

    /* Start server */
    if (wolfSSL_NewThread(&serverThread, server_thread, NULL) != 0) {
        printf("Failed to start server thread\n");
        return -1;
    }

#ifdef WOLFSSL_STATIC_MEMORY
    if (wc_LoadStaticMemory(&HEAP_HINT_CLIENT, gMemoryClient,
                               sizeof(gMemoryClient),
                               WOLFMEM_GENERAL | WOLFMEM_TRACK_STATS, 1) != 0) {
        printf("unable to load static memory");
        ret = -1;
    }

    if (ret == 0)
#endif
    {
        /* Client connection */
        ret = wolfssl_client_new(&client_ctx, &client_ssl);
    }

    while (ret == 0) {
        ret = wolfssl_client_connect(client_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(client_ssl))
            break;
        k_sleep(Z_TIMEOUT_TICKS(10));
    }

    if (ret == 0) {
        printf("Handshake complete\n");

        /* Send HTTP request */
        ret = wolfssl_send(client_ssl, msgHTTPGet);
    }
    /* Receive HTTP response */
    while (ret == 0) {
        k_sleep(Z_TIMEOUT_TICKS(10));
        ret = wolfssl_recv(client_ssl);
    }
    if (ret == 1)
        ret = 0;

    printf("Client Return: %d\n", ret);
    printf("Client Error: %d\n", wolfSSL_get_error(client_ssl, ret));

    if (wolfSSL_JoinThread(serverThread) != 0) {
        printf("Failed to join server thread\n");
        return -1;
    }

#ifdef WOLFSSL_STATIC_MEMORY
    printf("Client Memory Stats\n");
#endif
    wolfssl_memstats(client_ssl);

#if defined(WOLFSSL_HAVE_PSA) && defined(HAVE_PK_CALLBACKS)
    if (client_ssl != NULL) {
        wolfSSL_free_psa_ctx(&client_psa_ctx);
    }
#endif
    wolfssl_free(client_ctx, client_ssl);

    wolfSSL_Cleanup();

    printf("Done\n");

    return (ret == 0) ? 0 : 1;
}

