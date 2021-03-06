ifeq ($(SCIDB),) 
  X := $(shell which scidb 2>/dev/null)
  ifneq ($(X),)
    X := $(shell dirname ${X})
    SCIDB := $(shell dirname ${X})
  endif
endif

# A development environment will have SCIDB_VER defined, and SCIDB
# # will not be in the same place... but the 3rd party directory *will*
# # be, so build it using SCIDB_VER .
ifeq ($(SCIDB_VER),)
  SCIDB_3RDPARTY = $(SCIDB)
else
  SCIDB_3RDPARTY = /opt/scidb/$(SCIDB_VER)
endif

ifeq ($(SCIDB_THIRDPARTY_PREFIX),) 
  SCIDB_THIRDPARTY_PREFIX := $(SCIDB_3RDPARTY)
endif

CFLAGS=-pedantic -W -Wextra -Wall -Wno-variadic-macros -Wno-strict-aliasing -Wno-long-long -Wno-unused-parameter -fPIC -D_STDC_FORMAT_MACROS -Wno-system-headers -isystem -O2 -g -DNDEBUG -ggdb3  -D_STDC_LIMIT_MACROS
CXXFLAGS=-std=c++11 -std=gnu++14 -fext-numeric-literals -pedantic -W -Wextra -Wall -Wno-variadic-macros -Wno-strict-aliasing \
         -Wno-long-long -Wno-unused-parameter -Wno-unused -ggdb3 -O2 -fPIC
INC=-I. -DPROJECT_ROOT="\"$(SCIDB)\"" -I"$(SCIDB_THIRDPARTY_PREFIX)/3rdparty/boost/include/" -I"$(SCIDB)/include"

LIBS=-shared -Wl,-soname,libsuperfunpack.so -lpcre -L. -L"$(SCIDB_THIRDPARTY_PREFIX)/3rdparty/boost/lib" -L"$(SCIDB)/lib" -Wl,-rpath,$(SCIDB)/lib:$(RPATH) -lm

# Compiler settings for SciDB version >= 15.7
ifneq ("$(wildcard /usr/bin/g++-4.9)","")
  CC := "/usr/bin/gcc-4.9"
  CXX := "/usr/bin/g++-4.9"
  CXXFLAGS+=-std=c++14 -DCPP14
else
  ifneq ("$(wildcard /opt/rh/devtoolset-3/root/usr/bin/gcc)","")
    CC := "/opt/rh/devtoolset-3/root/usr/bin/gcc"
    CXX := "/opt/rh/devtoolset-3/root/usr/bin/g++"
    CXXFLAGS+=-std=c++14 -DCPP14
  endif
endif

all:
	@if test ! -d "$(SCIDB)"; then echo  "Error. Try:\n\nmake SCIDB=<PATH TO SCIDB INSTALL PATH>"; exit 1; fi
	$(MAKE) -C R
	$(CC) $(CFLAGS) -c pcrs.c -lpcre
	$(CXX) $(CXXFLAGS) $(INC) -o libsuperfunpack.so pcrs.o R/bd0.o  R/dbinom.o  R/dhyper.o  R/stirlerr.o plugin.cpp superfunpack.cpp $(LIBS)
	@echo "Now copy libsuperfunpack.so to your SciDB lib/scidb/plugins directory and restart SciDB."

clean:
	$(MAKE) -C R clean
	rm -f *.so *.o
