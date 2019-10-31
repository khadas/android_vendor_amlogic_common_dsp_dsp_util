#
## makefile for libgdc and gdc_test
#
ifeq (,$(CROSS_PREFIX))
CROSS_PREFIX=armv8l-linux-gnueabihf-
endif
CC?=$(CROSS_PREFIX)gcc
CXX?=$(CROSS_PREFIX)g++
CFLAGS = -fPIC -I./include/ -I./mp3tools/ -Wall -Wno-unused-function -Werror=sequence-point -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=unused-variable -Werror=unused-parameter -D_GNU_SOURCE
#CPPFLAGS+= $(CFLAGS)
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
HIFI4_TOOL_SRC = $(wildcard hifi4_tool/*.c)
HIFI4_TOOL_OBJ = $(patsubst %c, %o, $(HIFI4_TOOL_SRC))
HIFI4_TOOL = hifi4_media_tool

HIFI4RPC_CLIENT_TEST_SRC = $(wildcard hifi4rpc_test/*.c)
HIFI4RPC_CLIENT_TEST_SRC_CPP = $(wildcard hifi4rpc_test/*.cpp)
HIFI4RPC_CLIENT_TEST_OBJ += $(patsubst %c, %o, $(HIFI4RPC_CLIENT_TEST_SRC))
HIFI4RPC_CLIENT_TEST_OBJ += $(patsubst %cpp, %o, $(HIFI4RPC_CLIENT_TEST_SRC_CPP))
HIFI4RPC_CLIENT_TEST = hifi4rpc_client_test

DSP_UTIL_SRC = $(wildcard src/*.c)
DSP_UTIL_OBJ = $(patsubst %c, %o, $(DSP_UTIL_SRC))
DSP_UTIL = dsp_util

HIFI4RPC_TEST_SRC = test/rpc_test.c
HIFI4RPC_TEST_OBJ = test/rpc_test.o
HIFI4RPC_TEST = hifi4_rpc_test

OBJS = $(HIFI4_TOOL_OBJ) $(HIFI4RPC_CLIENT_TEST_OBJ) $(LIBHIFI4RPC_CLIENT_OBJ) $(LIBHIFI4RPC_OBJ) $(HIFI4RPC_TEST_OBJ) $(LIBMP3TOOLS_OBJ) $(DSP_UTIL_OBJ)
LIBS = $(LIBHIFI4RPC_CLIENT) $(LIBHIFI4RPC) $(LIBMP3TOOLS)
APPS = $(HIFI4_TOOL) $(HIFI4RPC_CLIENT_TEST) $(HIFI4RPC_TEST) $(DSP_UTIL)

EXPORT_HEADERFILES = ./include/rpc_client_mp3.h \
	./include/rpc_client_shm.h \
	./include/rpc_client_aipc.h \
	./include/aipc_type.h \
	./include/pvmp3decoder_api.h \
	./include/pvmp3_audio_type_defs.h \
	./include/pvmp3_dec_defs.h \
	./include/ipc_cmd_type.h \
        ./include/FDK_audio.h \
	./include/aacdecoder_lib.h \
	./include/aipc_type.h \
	./include/genericStds.h \
	./include/ipc_cmd_type.h \
	./include/machine_type.h \
	./include/rpc_client_aac.h \
	./include/syslib_channelMapDescr.h \
	./include/aml_wakeup_api.h \
	./include/aml_audio_util.h

# rules
all: $(LIBS) $(APPS)

$(HIFI4_TOOL): $(LIBHIFI4RPC_CLIENT) $(LIBHIFI4RPC)
$(HIFI4RPC_CLIENT_TEST): $(LIBHIFI4RPC_CLIENT) $(LIBHIFI4RPC) $(LIBMP3TOOLS)
$(HIFI4RPC_TEST): $(LIBHIFI4RPC)

%.o:%.c
	$(CC) -c  $(CFLAGS) $^ -o $@

%.o:%.cpp
	$(CXX) -c  $(CFLAGS) $^ -o $@

#libraries compile
$(LIBHIFI4RPC_CLIENT): $(LIBHIFI4RPC_CLIENT_OBJ)
	$(CC) -shared $(CFLAGS) $^ -o $(LIBHIFI4RPC_CLIENT)

$(LIBMP3TOOLS): $(LIBMP3TOOLS_OBJ)
	$(CXX) -shared  $(CFLAGS) $^ -o $(LIBMP3TOOLS)

$(LIBHIFI4RPC): $(LIBHIFI4RPC_OBJ)
	$(CC) -shared  $(CFLAGS) $^ -o $(LIBHIFI4RPC)

#applications compile
$(HIFI4_TOOL): $(HIFI4_TOOL_OBJ)
	$(CC) $^ -I./ -L$(LIBDIR) -lhifi4rpc_client -lhifi4rpc -lpthread $(CFLAGS) -o $@

$(HIFI4RPC_CLIENT_TEST): $(HIFI4RPC_CLIENT_TEST_OBJ)
	$(CXX) $^ -I./ -L$(LIBDIR) -lhifi4rpc_client -lhifi4rpc -lmp3tools -lpthread $(CFLAGS) -o $@

$(HIFI4RPC_TEST): $(HIFI4RPC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lhifi4rpc $(CFLAGS) -o $@

$(DSP_UTIL): $(DSP_UTIL_OBJ)
	$(CC) $^ $(CFLAGS) -o $@


#clean targets
.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBS) $(APPS)

install:
	mkdir -p $(STAGING_DIR)/usr/include/aml_dsp
	-install -m 644 $(EXPORT_HEADERFILES) $(STAGING_DIR)/usr/include/aml_dsp
	-install -m 644 $(LIBS) $(STAGING_DIR)/usr/lib
	-install -m 644 $(LIBS) $(TARGET_DIR)/usr/lib
	-install -m 755 $(APPS) $(TARGET_DIR)/usr/bin
