
CC =   cc
CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g
CPP =   cc -E
LINK = $(CC)


CORE_DEPS = src/core/nginx.h src/core/ngx_config.h

CORE_INCS = -I src/core \
	-I src/event \
	-I src/event/modules \
	-I src/os/unix \
	-I /home/nginx-1.13.0/thirdparty/pcre-8.40 \
	-I /home/nginx-1.13.0/thirdparty/zlib-1.2.11 \
	-I objs


build: binary 

binary: objs/nginx

objs/nginx: objs/src/core/nginx.o
	$(LINK)  -o objs/nginx objs/src/core/nginx.o

objs/src/core/nginx.o:  $(CORE_DEPS)
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/nginx.o src/core/nginx.c 
 
