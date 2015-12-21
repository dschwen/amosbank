all: amosbank

amosbank: amosbank.cc
	g++ -o amosbank amosbank.cc -lpng

amosextract: amosextract.cc
	g++ -o amosextract amosextract.cc
