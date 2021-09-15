TVMSRCS = chunk.c \
          debug.c \
          vm-main.c \
          memory.c \
          value.c \
          vm.c

TROLLCSRCS = compiler.c \
             scanner.c \
             trollc-main.c

tvm: ${TVMSRCS}
	gcc -o tvm ${TVMSRCS}

trollc: ${TROLLCSRCS}
	gcc -o trollc ${TROLLCSRCS}

clean:
	rm -rf *~ tvm

