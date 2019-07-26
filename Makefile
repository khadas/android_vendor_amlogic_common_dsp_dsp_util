#
## makefile for libgdc and gdc_test
#
ifeq (,$(CROSS_PREFIX))
CROSS_PREFIX=armv8l-linux-gnueabihf-
endif
CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++
CFLAGS = -I ./include/ -I ./tools/
LIBDIR:= .

LIBHIFICODEC_SRC += $(wildcard src/*.c)
LIBHIFICODEC_SRC += $(wildcard tools/*.c)
LIBHIFICODEC_SRC_CPP = $(wildcard tools/*.cpp)
LIBHIFICODEC_OBJ += $(patsubst %c, %o, $(LIBHIFICODEC_SRC))
LIBHIFICODEC_OBJ += $(patsubst %cpp, %o, $(LIBHIFICODEC_SRC_CPP))
LIBHIFICODEC = libhifi4codec.so


HIFICODEC_TEST_SRC_CPP = $(wildcard test/*.cpp)
HIFICODEC_TEST_OBJ = $(patsubst %cpp, %o, $(HIFICODEC_TEST_SRC_CPP))
HIFICODEC_TEST = hifi4codec_test

HIFI4APP_SRC = test/hifi4app.c
HIFI4APP_OBJ = test/hifi4app.o
HIFI4APP = hifi4app

LIBHIFI4RPC_SRC += $(wildcard rpc/*.c)
LIBHIFI4RPC_OBJ += $(patsubst %c, %o, $(LIBHIFI4RPC_SRC))
LIBHIFI4RPC = libhifi4rpc.so
HIFI4RPC_TEST_SRC = test/rpc_test.c
HIFI4RPC_TEST_OBJ = test/rpc_test.o
HIFI4RPC_TEST = hifi4_rpc_test

OBJS = $(HIFI4APP_OBJ) $(HIFICODEC_TEST_OBJ) $(LIBHIFICODEC_OBJ) $(LIBHIFI4RPC_OBJ) $(HIFI4RPC_TEST_OBJ)

# rules
all: $(LIBHIFICODEC) $(HIFICODEC_TEST) $(HIFI4APP) $(LIBHIFI4RPC) $(HIFI4RPC_TEST)

$(HIFICODEC_TEST): $(LIBHIFICODEC)

%o:%c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@

%o:%cpp
	$(CXX) -c -fPIC $(CFLAGS) $^ -o $@

$(LIBHIFICODEC): $(LIBHIFICODEC_OBJ)
	$(CC) -shared -fPIC $(CFLAGS) $^ -o $(LIBHIFICODEC)

$(HIFICODEC_TEST): $(HIFICODEC_TEST_OBJ)
	$(CXX) $^ -L$(LIBDIR) -lhifi4codec $(CFLAGS) -o $@

$(HIFI4APP): $(HIFI4APP_OBJ)
	$(CC) $^ -o $@

$(LIBHIFI4RPC): $(LIBHIFI4RPC_OBJ)
	$(CC) -shared -fPIC -I ./include $^ -o $(LIBHIFI4RPC)

$(HIFI4RPC_TEST): $(HIFI4RPC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lhifi4rpc $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(HIFICODEC_TEST) $(LIBHIFICODEC) $(HIFI4APP) \
		${LIBHIFI4RPC} $(HIFI4RPC_TEST)
