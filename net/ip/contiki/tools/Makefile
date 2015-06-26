all: tunslip6 echo-client

tunslip6: tunslip6.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) tunslip6.c

echo-client: echo-client.o
	$(CC) -o $@ $(CFLAGS) $(LIBS) echo-client.c
