APPNAME=venomous

CXX=clang++
CXXFLAGS=-std=c++14 -m64 -maes -O0 -fno-exceptions
ASMFLAGS=-S -fverbose-asm

all: build/$(APPNAME).exe
	build/$(APPNAME).exe

build/$(APPNAME).exe: main.cpp key_value_pair.h utils/murmur3.h tmp_utils.h unordered_map.h engine.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).exe $(LDLIBS)
	# $(CXX) $(CXXFLAGS) $(ASMFLAGS) $(LDFLAGS) $< -o build/$(APPNAME).txt $(LDLIBS) build/farmhash.o

clean:
	rm -f build/*.o build/*.exe build/*.txt


