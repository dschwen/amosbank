all: amosbank

amosbank: amosbank.cc
	g++ -o amosbank amosbank.cc  -lpng -lz

amosextract: amosextract.cc
	g++ -o amosextract amosextract.cc

