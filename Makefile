PROGRAM=suzume.cgi
OBJS=build/main.o build/encodeu8.o build/urlencoded.o build/runcgi.o

MAIN_DEPS=src/sqlite3pp.hpp src/wjson.hpp src/wmustache.hpp\
	 src/encodeu8.hpp src/http.hpp\
	 src/suzume_data.hpp src/suzume_view.hpp
ENCODEU8_DEPS=src/encodeu8.hpp
URLENCODED_DEPS=src/http.hpp src/encodeu8.hpp
RUNCGI_DEPS=src/http.hpp src/runcgi.hpp

CXX=clang++
CXXFLAGS=-std=c++11 -Wall -O2
LDFLAGS=-std=c++11
LIBS=-lsqlite3

.PHONY: all clean

all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(CXX) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS)

build/main.o : src/main.cpp $(MAIN_DEPS)
	$(CXX) $(CXXFLAGS) -o build/main.o -c src/main.cpp

build/encodeu8.o : src/encodeu8.cpp $(ENCODEU8_DEPS)
	$(CXX) $(CXXFLAGS) -o build/encodeu8.o -c src/encodeu8.cpp

build/urlencoded.o : src/urlencoded.cpp $(URLENCODED_DEPS)
	$(CXX) $(CXXFLAGS) -o build/urlencoded.o -c src/urlencoded.cpp

build/runcgi.o : src/runcgi.cpp $(RUNCGI_DEPS)
	$(CXX) $(CXXFLAGS) -o build/runcgi.o -c src/runcgi.cpp

clean :
	rm -f $(PROGRAM) $(OBJS)
