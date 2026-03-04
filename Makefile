CC := gcc
WCC := clang-19
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
WCFLAGS := --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all

VERSION_PATCH=0
VERSION_UPDATE=0
VERSION_RELEASE=0
VERSION_TAG="\"-rc\""

Sources := $(wildcard src/*.c)
Headers := $(wildcard src/*.h)
WModules := $(notdir $(wildcard src/modules/*))
Deps := \
	bin/wamr/libiwasm.a

WTargets := $(addprefix bin/modules/,$(addsuffix .wasm,$(WModules)))
TAGS_TARGET := \
	$(shell find src -path 'src/modules' -prune -o -name '*.[ch]' -print) \
	$(shell find wamr/core -name '*.[ch]' -print)

all: \
	TAGS \
	femail/debian/debfemail \
	femail/scratch/debfemail-st \
	bin/femail bin/femail-st \
	$(WTargets)

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

bin/femail: $(Sources) $(Deps) $(Headers) 
	$(CC) -DAPP_NAME="\"Femail Mail System\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) -O2 $(Sources) $(Deps) $(LIBS) -o $@

bin/femail-st: $(Sources) $(Deps) $(Headers)
	$(CC) -DAPP_NAME="\"Femail Mail System(Static)\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) -O2 -static $(Sources) $(Deps) $(LIBS) -o $@

bin/debfemail: $(Sources) $(Deps) $(Headers)
	$(CC) -DAPP_NAME="\"Femail Debug Mail System\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) $(DEBUGFLAGS) -O1 $(Sources) $(Deps) $(LIBS) -o $@

bin/debfemail-st: $(Sources) $(Deps) $(Headers)
	$(CC) -DAPP_NAME="\"Femail Debug Mail System(Static)\"" \
	-DVERSION_PATCH=$(VERSION_PATCH) \
	-DVERSION_UPDATE=$(VERSION_UPDATE) \
	-DVERSION_RELEASE=$(VERSION_RELEASE) \
	-DVERSION_TAG=$(VERSION_TAG) \
	$(CFLAGS) $(STDEBUGFLAGS) -O1 $(Sources) $(Deps) $(LIBS) -o $@

deps: bin/wamr/libiwasm.a

bin/wamr/libiwasm.a: bin/wamr/Makefile
	make -C bin/wamr -j$(shell nproc)

bin/wamr/Makefile:
	mkdir -p bin/wamr
	cmake -Swamr -Bbin/wamr \
		-DWAMR_BUILD_PLATFORM=linux \
		-DWAMR_BUILD_INTERP=1 \
		-DWAMR_BUILD_FAST_INTERP=1 \
		-DWAMR_BUILD_AOT=0 \
		-DWAMR_BUILD_JIT=0 \
		-DWAMR_BUILD_FAST_JIT=0 \
		-DWAMR_BUILD_LIBC_BUILTIN=0 \
		-DWAMR_BUILD_LIBC_WASI=0 \
		-DWAMR_BUILD_MULTI_MODULE=0 \
		-DWAMR_BUILD_SHARED_MEMORY=0 \
		-DWAMR_BUILD_THREAD_MGR=0 \
		-DWAMR_BUILD_LIB_PTHREAD=0 \
		-DWAMR_BUILD_GC=0 \
		-DWAMR_BUILD_SIMD=1 \
		-DWAMR_BUILD_REF_TYPES=1 \
		-DWAMR_BUILD_TAIL_CALL=1 \
		-DWAMR_BUILD_CUSTOM_NAME_SECTION=1 \
		-DWAMR_BUILD_DUMP_CALL_STACK=1 \
		-DWAMR_DISABLE_HW_BOUND_CHECK=1 \
		-DWAMR_DISABLE_STACK_HW_BOUND_CHECK=1 \
		-DWAMR_BUILD_SHRUNK_MEMORY=1 \
		-DWAMR_BUILD_MEMORY64=0;

bin/modules/%.wasm: src/modules/%/mod.c
	$(WCC) $(WCFLAGS) $< -o $@

TAGS: $(TAGS_TARGET)
	etags --declarations --members --ignore-indentation -o TAGS $^

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

.PHONY: all up down logs certs deps
