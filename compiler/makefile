APPNAME=venomous

CXX=clang++
CXXFLAGS=-std=c++14 -m64 -maes -O3 -fno-exceptions
ASMFLAGS=-S -fverbose-asm

all: build/$(APPNAME).exe
	build/$(APPNAME).exe

build/$(APPNAME).exe: main.cpp packed_cache_line.h utils/murmur3.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).exe $(LDLIBS)
	# $(CXX) $(CXXFLAGS) $(ASMFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).txt $(LDLIBS) build/farmhash.o

clean:
	rm -f build/*.o build/*.exe build/*.txt


