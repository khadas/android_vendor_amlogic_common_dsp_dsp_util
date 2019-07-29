#
## makefile for libgdc and gdc_test
#
ifeq (,$(CROSS_PREFIX))
CROSS_PREFIX=armv8l-linux-gnueabihf-
endif
CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++
CFLAGS = -I ./include/ -I ./mp3tools/
LIBDIR:= .

#libraries define
LIBHIFI4RPC_CLIENT_SRC += $(wildcard hifi4rpc_client/*.c)
LIBHIFI4RPC_CLIENT_OBJ += $(patsubst %c, %o, $(LIBHIFI4RPC_CLIENT_SRC))
LIBHIFI4RPC_CLIENT = libhifi4rpc_client.so

LIBMP3TOOLS_SRC += $(wildcard mp3tools/*.c)
LIBMP3TOOLS_SRC_CPP = $(wildcard mp3tools/*.cpp)
LIBMP3TOOLS_OBJ += $(patsubst %c, %o, $(LIBMP3TOOLS_SRC))
LIBMP3TOOLS_OBJ += $(patsubst %cpp, %o, $(LIBMP3TOOLS_SRC_CPP))
LIBMP3TOOLS = libmp3tools.so

LIBHIFI4RPC_SRC += $(wildcard hifi4rpc/*.c)
LIBHIFI4RPC_OBJ += $(patsubst %c, %o, $(LIBHIFI4RPC_SRC))
LIBHIFI4RPC = libhifi4rpc.so

#applications define
HIFI4RPC_CLIENT_TEST_SRC_CPP = $(wildcard test/*.cpp)
HIFI4RPC_CLIENT_TEST_OBJ = $(patsubst %cpp, %o, $(HIFI4RPC_CLIENT_TEST_SRC_CPP))
HIFI4RPC_CLIENT_TEST = hifi4rpc_client_test

HIFI4APP_SRC = test/hifi4app.c
HIFI4APP_OBJ = test/hifi4app.o
HIFI4APP = hifi4app

HIFI4RPC_TEST_SRC = test/rpc_test.c
HIFI4RPC_TEST_OBJ = test/rpc_test.o
HIFI4RPC_TEST = hifi4_rpc_test

OBJS = $(HIFI4APP_OBJ) $(HIFI4RPC_CLIENT_TEST_OBJ) $(LIBHIFI4RPC_CLIENT_OBJ) $(LIBHIFI4RPC_OBJ) $(HIFI4RPC_TEST_OBJ) $(LIBMP3TOOLS_OBJ)
LIBS = $(LIBHIFI4RPC_CLIENT) $(LIBHIFI4RPC) $(LIBMP3TOOLS)
APPS = $(HIFI4RPC_CLIENT_TEST) $(HIFI4APP) $(HIFI4RPC_TEST)

# rules
all: $(LIBS) $(APPS)

$(HIFI4RPC_CLIENT_TEST): $(LIBHIFI4RPC_CLIENT) $(LIBHIFI4RPC) $(LIBMP3TOOLS)
$(HIFI4RPC_TEST): $(LIBHIFI4RPC)

%o:%c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@

%o:%cpp
	$(CXX) -c -fPIC $(CFLAGS) $^ -o $@

#libraries compile
$(LIBHIFI4RPC_CLIENT): $(LIBHIFI4RPC_CLIENT_OBJ)
	$(CC) -shared -fPIC $(CFLAGS) $^ -o $(LIBHIFI4RPC_CLIENT)

$(LIBMP3TOOLS): $(LIBMP3TOOLS_OBJ)
	$(CXX) -shared -fPIC $(CFLAGS) $^ -o $(LIBMP3TOOLS)

$(LIBHIFI4RPC): $(LIBHIFI4RPC_OBJ)
	$(CC) -shared -fPIC -I ./include $^ -o $(LIBHIFI4RPC)

#applications compile
$(HIFI4RPC_CLIENT_TEST): $(HIFI4RPC_CLIENT_TEST_OBJ)
	$(CXX) $^ -I./ -L$(LIBDIR) -lhifi4rpc_client -lhifi4rpc -lmp3tools $(CFLAGS) -o $@

$(HIFI4APP): $(HIFI4APP_OBJ)
	$(CC) $^ -o $@

$(HIFI4RPC_TEST): $(HIFI4RPC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lhifi4rpc $(CFLAGS) -o $@

#clean targets
.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBS) $(APPS)
