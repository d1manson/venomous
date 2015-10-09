APPNAME=venomous.exe

CXX=clang++
CXXFLAGS=-std=c++11

all: build/$(APPNAME)

build/$(APPNAME): build/main.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o build/$(APPNAME) $< $(LDLIBS)

build/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o build/main.o

clean:
	rm -f build/*.o build/*.exe

