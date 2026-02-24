CC := gcc
CONTAINER_TOOL=podman
CFLAGS := \
	-std=c23 -pthread \
	-Wall -Wextra -Werror -Wpedantic -pedantic-errors \
	-Wshadow -Wconversion -Wsign-conversion -Wcast-align -Wcast-qual \
	-Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
	-Wnull-dereference -Wpointer-arith -Wvla -Wwrite-strings \
	-Wredundant-decls -Warray-bounds -Wnonnull -Wfloat-equal \
	-Wduplicated-cond -Wduplicated-branches -Wlogical-op \
	-Wuseless-cast -Wstringop-overflow
LIBS := -lssl -lcrypto -lz -lzstd -ldl
DEBUGFLAGS := \
	-fsanitize=address,undefined,leak -fno-omit-frame-pointer \
	-fstack-protector-strong -fno-common -DDEBUG=1

STDEBUGFLAGS := \
	-static -fno-omit-frame-pointer \
	-fstack-protector-strong -fno-common -DDEBUG=1

VERSION_PATCH=0
VERSION_UPDATE=0
VERSION_RELEASE=0
VERSION_TAG="\"-rc\""

Sources := \
	src/femail.c src/comm.c src/master.c src/dns.c src/smtp.c \
	src/http.c

all: femail/debian/debfemail \
	femail/scratch/debfemail-st \
	bin/femail bin/femail-st

up: femail/debian/debfemail femail/scratch/debfemail-st all
	$(CONTAINER_TOOL) build -t femailbase .
	$(CONTAINER_TOOL)-compose up -d --build

down:
	$(CONTAINER_TOOL)-compose down -t 3

logs:
	$(CONTAINER_TOOL)-compose logs --timestamp --names -f

femail/debian/debfemail: bin/debfemail
	install $< $@

femail/scratch/debfemail-st: bin/debfemail-st
	install $< $@

bin/femail: $(Sources)
	$(CC) -DAPP_NAME="\"Femail Mail System\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) -O2 $(Sources) $(LIBS) -o $@

bin/femail-st: $(Sources)
	$(CC) -DAPP_NAME="\"Femail Mail System(Static)\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) -O2 -static $(Sources) $(LIBS) -o $@

bin/debfemail: $(Sources)
	$(CC) -DAPP_NAME="\"Femail Debug Mail System\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) $(DEBUGFLAGS) -O1 $(Sources) $(LIBS) -o $@

bin/debfemail-st: $(Sources)
	$(CC) -DAPP_NAME="\"Femail Debug Mail System(Static)\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) $(STDEBUGFLAGS) -O1 $(Sources) $(LIBS) -o $@

certs: femail/debian/mail.mailey.femail.crt femail/scratch/mail.failey.femail.crt

femail/debian/mail.mailey.femail.crt:
	openssl req -x509 -newkey rsa:2048 -sha256 -days 365 -nodes \
		-keyout ca/mail.mailey.femail.key \
		-out ca/mail.mailey.femail.crt \
		-subj "/CN=mail.mailey.femail" \
		-addext "subjectAltName=DNS:mail.mailey.femail"
	install -T ca/mail.mailey.femail.key femail/debian/mail.mailey.femail.key
	install -T ca/mail.mailey.femail.crt femail/debian/mail.mailey.femail.crt
femail/scratch/mail.failey.femail.crt:
	openssl req -x509 -newkey rsa:2048 -sha256 -days 365 -nodes \
		-keyout ca/mail.failey.femail.key \
		-out ca/mail.failey.femail.crt \
		-subj "/CN=mail.failey.femail" \
		-addext "subjectAltName=DNS:mail.failey.femail"
	install -T ca/mail.failey.femail.key femail/scratch/mail.failey.femail.key
	install -T ca/mail.failey.femail.crt femail/scratch/mail.failey.femail.crt

.PHONY: all up down logs certs
