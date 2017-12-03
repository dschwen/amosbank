CXX ?= g++
LIBPNG_CONFIG ?= libpng-config
LDFLAGS += $(shell $(LIBPNG_CONFIG) --cflags --ldflags)

all: amosbank

amosbank: amosbank.cc
	$(CXX) -o amosbank amosbank.cc $(LDFLAGS)

amosextract: amosextract.cc
	$(CXX) -o amosextract amosextract.cc

clean:
	rm -f amosbank amosextract
