TVMSRCS = chunk.c \
          debug.c \
          vm-main.c \
          memory.c \
          value.c \
          vm.c

TROLLCSRCS = chunk.c \
             compiler.c \
             memory.c \
             scanner.c \
             trollc-main.c \
             value.c

DECOMSRCS = chunk.c \
            debug.c \
            decom-main.c \
            memory.c \
            value.c

tvm: ${TVMSRCS}
	gcc -o tvm ${TVMSRCS}

trollc: ${TROLLCSRCS}
	gcc -o trollc ${TROLLCSRCS}

decom: ${DECOMSRCS}
	gcc -o decom ${DECOMSRCS}

clean:
	rm -rf *~ tvm

