ROOTCONFIG   := root-config
ROOTCINT     := rootcint
ARCH         := $(shell $(ROOTCONFIG) --arch)
ROOTCFLAGS   := $(shell $(ROOTCONFIG) --cflags)
ROOTLDFLAGS  := $(shell $(ROOTCONFIG) --ldflags)
ROOTGLIBS    := $(shell $(ROOTCONFIG) --libs)

MYSQLCONIG   := mysql_config
MYSQLCFLAGS  := $(shell $(MYSQLCONIG) --include)
MYSQLLDFLAGS := $(shell $(MYSQLCONIG) --libs)

CXX           = g++
CXXFLAGS      = -O3 -Wall -fPIC
LD            = g++
LDFLAGS       = -O3
SOFLAGS       = -shared

CXXFLAGS     += $(ROOTCFLAGS)
LDFLAGS      += $(ROOTLDFLAGS) $(ROOTGLIBS)

CXXFLAGS     += -I$(BOOST)

CXXFLAGS     += $(MYSQLCFLAGS)
LDFLAGS      += $(MYSQLLDFLAGS)

DATASTRUCTO   = DataStruct.o DataStructDict.o
DATASTRUCTSO  = libAnaUtil.so

OBJS          = $(DATASTRUCTO) 
SLIBS         = $(DATASTRUCTSO) 

all:            $(SLIBS)

.SUFFIXES: .cxx .o

$(DATASTRUCTSO):  $(DATASTRUCTO)
	$(LD) $^ -o $@  $(SOFLAGS) $(LDFLAGS)
	@echo "$@ done."

.SUFFIXES: .cxx

DataStructDict.cxx: DataStruct.h LinkDef.h
	@echo "Generating dictionary for $@ ..."
	$(ROOTCINT) -f $@ -c $^

.cxx.o:
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean

clean:
	@echo "Cleanning everything ... "
	@rm $(OBJS) $(SLIBS) *Dict.cxx *Dict.h
