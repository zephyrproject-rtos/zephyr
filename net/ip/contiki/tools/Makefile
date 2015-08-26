all: tunslip6 tunslip echo-client echo-server monitor_15_4

tunslip6: tunslip6.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) tunslip6.c

tunslip: tunslip.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) tunslip.c

echo-client: echo-client.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) echo-client.c

echo-server: echo-server.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) echo-server.c

TINYDTLS = $(HOME)/src/tinydtls/tinydtls-0.8.2
TINYDTLS_CFLAGS = -I$(TINYDTLS) -DDTLSv12 -DWITH_SHA256 -DDTLS_ECC -DDTLS_PSK
TINYDTLS_LIB = $(TINYDTLS)/libtinydtls.a

dtls-client.o: dtls-client.c
	$(CC) -c -o $@ $(CFLAGS) $(TINYDTLS_CFLAGS) dtls-client.c

dtls-client: dtls-client.o $(TINYDTLS_LIB)
	$(CC) -o $@ $(LIBS) dtls-client.o $(TINYDTLS_LIB)

dtls-server.o: dtls-server.c
	$(CC) -c -o $@ $(CFLAGS) $(TINYDTLS_CFLAGS) dtls-server.c

dtls-server: dtls-server.o $(TINYDTLS_LIB)
	$(CC) -o $@ $(LIBS) dtls-server.o $(TINYDTLS_LIB)

GLIB=`pkg-config --cflags --libs glib-2.0`

monitor_15_4.o: monitor_15_4.c
	$(CC) -c -o $@ $(CFLAGS) $(GLIB) monitor_15_4.c

monitor_15_4: monitor_15_4.o
	$(CC) -o $@ $(LIBS) monitor_15_4.o $(GLIB)

LIBCOAP = $(HOME)/src/coap/libcoap
LIBCOAP_CFLAGS = -I$(LIBCOAP)/include -I$(LIBCOAP) -DWITH_POSIX
LIBCOAP_LIB = $(LIBCOAP)/.libs/libcoap-1.a

coap-client.o: coap-client.c
	$(CC) -c -o $@ $(CFLAGS) $(TINYDTLS_CFLAGS) $(LIBCOAP_CFLAGS) coap-client.c

coap-client: coap-client.o $(TINYDTLS_LIB) $(LIBCOAP_LIB)
	$(CC) -o $@ $(LIBS) coap-client.o $(TINYDTLS_LIB) $(LIBCOAP_LIB)
