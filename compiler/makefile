APPNAME=venomous

CXX=clang++
CXXFLAGS=-std=c++14 -m64 -maes -O3 -fno-exceptions
ASMFLAGS=-S -fverbose-asm

all: build/$(APPNAME).exe
	build/$(APPNAME).exe

build/$(APPNAME).exe: main.cpp packed_cache_line.h build/farmhash.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).exe $(LDLIBS) build/farmhash.o
	# $(CXX) $(CXXFLAGS) $(ASMFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).txt $(LDLIBS) build/farmhash.o

build/farmhash.o: farmhash.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o build/farmhash.o $(LDLIBS)


clean:
	rm -f build/*.o build/*.exe build/*.txt


