#
## makefile for libgdc and gdc_test
#
ifeq (,$(CC))
CC=armv8l-linux-gnueabihf-gcc
endif
ifeq (,$(CXX))
CXX=armv8l-linux-gnueabihf-g++
endif
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
HIFICODEC_TEST = hificodec_test

HIFI4APP_SRC = test/hifi4app.c
HIFI4APP_OBJ = test/hifi4app.o
HIFI4APP = hifi4app

OBJS = $(HIFIAPP_OBJ) $(HIFICODEC_TEST_OBJ) $(LIBHIFICODEC_OBJ)

# rules
all: $(LIBHIFICODEC) $(HIFICODEC_TEST) $(HIFIAPP)

$(HIFICODEC_TEST): $(LIBHIFICODEC)

%o:%c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@

%o:%cpp
	$(CXX) -c -fPIC $(CFLAGS) $^ -o $@

$(LIBHIFICODEC): $(LIBHIFICODEC_OBJ)
	$(CC) -shared -fPIC $(CFLAGS) $^ -o $(LIBHIFICODEC)

$(HIFICODEC_TEST): $(HIFICODEC_TEST_OBJ)
	$(CXX) $^ -L$(LIBDIR) -lhifi4codec $(CFLAGS) -o $@

$(HIFIAPP): $(HIFIAPP_OBJ)
	$(CC) $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(HIFICODEC_TEST) $(LIBHIFICODEC) $(HIFI4APP)
