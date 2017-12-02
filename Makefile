LIBPNG_CONFIG ?= libpng-config

all: amosbank

amosbank: amosbank.cc
	g++ -o amosbank amosbank.cc `${LIBPNG_CONFIG} --cflags --ldflags`

amosextract: amosextract.cc
	g++ -o amosextract amosextract.cc

clean:
	rm -f amosbank amosextract
