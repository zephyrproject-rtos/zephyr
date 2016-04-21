PROGRAM=../profterm
GCC=gcc

$(PROGRAM): term.c
	$(GCC) term.c -Wall -o $(PROGRAM) -lpthread

default: $(PROGRAM)

clean:
	rm -rf $(PROGRAM)
