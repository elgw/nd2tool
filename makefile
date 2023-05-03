CC=gcc -std=gnu99

MANPATH?=/usr/share/man/man1/
DESTDIR?=/usr/local/bin/

LDFLAGS+=-flto \
-lcjson \
-llimfile \
-lnd2readsdk-shared \
-lm \
-Llib \
-ltiff

CFLAGS=-Wall -Wextra -std=gnu99

DBG?=0

ifeq ($(DBG), 1)
CFLAGS+=-g3
else
CFLAGS+=-O3 -DNDEBUG
endif

ANA?=0
ifeq ($(ANA), 1)
CFLAGS+=-fanalyzer
endif

SAN?=0
ifeq ($(SAN), 1)
CFLAGS+=-fsanitize=address,undefined,leak \
-fsanitize-undefined-trap-on-error
# santitize does not play with valgrind
endif

files=src/nd2tool.c \
src/tiff_util.c \
src/json_util.c \
src/srgb_from_lambda.c

shared=lib/liblimfile.so \
lib/libnd2readsdk-shared.so

inc=-Iinclude/

LDFLAGS+=

bin/nd2tool-linux-amd64: $(files)
	gcc $(CFLAGS) $(files) $(shared) $(LDFLAGS) $(inc) -o bin/nd2tool-linux-amd64


valgrind: bin/nd2tool-linux-amd64
	valgrind --leak-check=full bin/nd2tool-linux-amd64 /srv/secondary/ki/deconwolf/20220502_huygens_psf/quentin_bs2_100_bead/iiQV003_20220429_1.nd2


install: bin/nd2tool-linux-amd64
	sudo cp bin/nd2tool-linux-amd64 /usr/local/bin/nd2tool
	sudo cp lib/liblimfile.so /usr/local/lib/
	sudo cp lib/libnd2readsdk-shared.so /usr/local/lib
	sudo ldconfig
	sudo cp doc/nd2tool.1 $(MANPATH)

clean:
	rm -f bin/nd2tool-*
