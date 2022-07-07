MANPATH?=/usr/share/man/man1/
DESTDIR?=/usr/local/bin/

LDFLAGS+=-flto -lcjson -llimfile -lnd2readsdk-shared -lm -Llib

files=src/nd2tool.c src/tiff_util.c src/json_util.c
shared=lib/liblimfile.so lib/libnd2readsdk-shared.so
inc=-Iinclude/

LDFLAGS+=-ltiff

CFLAGS=-Wall -Wextra -std=gnu99
CFLAGS_DEBUG=$(CFLAGS) -g3
CFLAGS_FAST=$(CFLAGS) -O3 -DNDEBUG

bin/nd2tool-linux-amd64: $(files)
	gcc $(CFLAGS_DEBUG) $(files) $(shared) $(LDFLAGS) $(inc) -o bin/nd2tool-linux-amd64

release: src/nd2tool.c
	gcc $(CFLAGS_FAST) $(files) $(shared) $(LDFLAGS) $(inc) -o bin/nd2tool-linux-amd64


test:
	./bin/nd2tool-linux-amd64 /srv/secondary/ki/deconwolf/20220502_huygens_psf/quentin_bs2_100_bead/iiQV003_20220429_1.nd2

valgrind:
	valgrind --leak-check=full bin/nd2tool-linux-amd64 /srv/secondary/ki/deconwolf/20220502_huygens_psf/quentin_bs2_100_bead/iiQV003_20220429_1.nd2

install: bin/nd2tool-linux-amd64
	sudo cp bin/nd2tool-linux-amd64 /usr/local/bin/nd2tool
	sudo cp lib/liblimfile.so /usr/local/lib/
	sudo cp lib/libnd2readsdk-shared.so /usr/local/lib
	sudo ldconfig
	sudo cp doc/nd2tool.1 $(MANPATH)

getsdk:
	wget -nc -P include/ https://raw.githubusercontent.com/tlambert03/nd2/main/src/sdk/Linux/x86_64/include/Nd2ReadSdk.h
	wget -nc -P lib/ https://raw.githubusercontent.com/tlambert03/nd2/main/src/sdk/Linux/x86_64/lib/liblimfile.so
	wget -nc -P lib/ https://raw.githubusercontent.com/tlambert03/nd2/main/src/sdk/Linux/x86_64/lib/libnd2readsdk-shared.so
