APPNAME=venomous.exe

CXX=clang++
CXXFLAGS=-std=c++14 -m64 -maes

all: build/$(APPNAME)
	build/$(APPNAME)

build/$(APPNAME): main.cpp variant.h build/farmhash.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o build/$(APPNAME) $(LDLIBS) build/farmhash.o

build/farmhash.o: farmhash.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o build/farmhash.o $(LDLIBS)


clean:
	rm -f build/*.o build/*.exe


