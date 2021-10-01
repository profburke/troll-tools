TVMSRCS = chunk.c \
          debug.c \
          object.c \
          vm-main.c \
          memory.c \
          value.c \
          vm.c

TROLLCSRCS = chunk.c \
             compiler.c \
             memory.c \
             object.c \
             scanner.c \
             trollc-main.c \
             value.c

DECOMSRCS = chunk.c \
            debug.c \
            decom-main.c \
            memory.c \
            object.c \
            value.c

all: tvm trollc decom

tvm: ${TVMSRCS}
	gcc -o tvm ${TVMSRCS}

trollc: ${TROLLCSRCS}
	gcc -o trollc ${TROLLCSRCS}

decom: ${DECOMSRCS}
	gcc -o decom ${DECOMSRCS}

clean:
	rm -rf *~ tvm trollc decom

