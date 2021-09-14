SRCS = chunk.c \
       debug.c \
       main.c \
       memory.c \
       value.c \
       vm.c

tvm: ${SRCS}
	gcc -o tvm ${SRCS}

clean:
	rm -rf *~ tvm

