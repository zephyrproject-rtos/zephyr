.PHONY: all clean
.SECONDARY:

CFLAGS = -march=native -O2 -g

EXECUTABLES = zephyr_adsp_logger

all: $(EXECUTABLES)

clean:
	rm -f $(EXECUTABLES)

%.o: %.c
	$(C) $(CFLAGS) -c $< -o $@
