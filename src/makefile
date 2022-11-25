CC=gcc
CFLAGS=-Wall -Wextra -g3
LDFLAGS=-lm

files=srgb_from_lambda.c srgb_from_lambda_ut.c
srgb_from_lambda_ut: $(files) makefile
	$(CC) $(CFLAGS) $(files) $(LDFLAGS) -o srgb_from_lambda_ut
