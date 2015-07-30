all: tunslip6 tunslip echo-client echo-server

tunslip6: tunslip6.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) tunslip6.c

tunslip: tunslip.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) tunslip.c

echo-client: echo-client.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) echo-client.c

echo-server: echo-server.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) echo-server.c
