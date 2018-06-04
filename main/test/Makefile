CC=clang -Wall -Wextra
.PHONY: test
test: test.o
	chmod +x test.o
	./test.o
test.o:	test.c
	$(CC) -o test.o test.c