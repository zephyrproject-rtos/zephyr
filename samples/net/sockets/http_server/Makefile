.PHONY: all clean check

ZEPHYR_BASE = $(HOME)/zephyrproject/zephyr
CFLAGS = -I. -I$(ZEPHYR_BASE)/include/
CFLAGS += -g -O0

all: http_server

index.html.gz: src/index.html
	cat $< | gzip -c > $@

index.html.gz.inc: index.html.gz
	xxd -i $< $@
	sed -i \
		-e '/.*index_html_gz_len.*/d' \
		-e '/.*index_html_gz.*/d' \
		-e '/.*}.*/d' \
		$@

http_server: src/main.c src/server.c index.html.gz.inc
	gcc $(CFLAGS) -o http_server src/main.c src/server.c $(ZEPHYR_BASE)/subsys/net/lib/http/http_parser.c $(ZEPHYR_BASE)/subsys/net/lib/http/http_parser_url.c

clean:
	rm -f http_server index.html.gz index.html.gz.inc

check: http_server
	./http_server
