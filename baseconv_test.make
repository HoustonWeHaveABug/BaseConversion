BASECONV_C_FLAGS=-c -O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

BASECONV_OBJS=baseconv.o baseconv_test.o

baseconv_test.exe: ${BASECONV_OBJS}
	gcc -o baseconv_test.exe ${BASECONV_OBJS}

baseconv.o: baseconv.c baseconv.h baseconv_test.make
	gcc ${BASECONV_C_FLAGS} -o baseconv.o baseconv.c

baseconv_test.o: baseconv_test.c baseconv.h baseconv_test.make
	gcc ${BASECONV_C_FLAGS} -o baseconv_test.o baseconv_test.c
