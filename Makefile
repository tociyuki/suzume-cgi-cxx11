PROGRAM=suzume.cgi
OBJS=build/main.o build/encode-utf8.o build/multipartformdata.o \
     build/urlencoded.o build/runcgi.o build/value.o build/setter.o \
     build/mustache.o

MAIN_DEPS=src/sqlite3pp.hpp src/value.hpp src/mustache.hpp\
	 src/encode-utf8.hpp src/http.hpp\
	 src/suzume_data.hpp src/suzume_view.hpp
ENCODEUTF8_DEPS=src/encode-utf8.hpp
MULTIAPART_DEPS=src/http.hpp src/encode-utf8.hpp
URLENCODED_DEPS=src/http.hpp src/encode-utf8.hpp
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

build/encode-utf8.o : src/encode-utf8.cpp $(ENCODEU8_DEPS)
	$(CXX) $(CXXFLAGS) -o build/encode-utf8.o -c src/encode-utf8.cpp

build/multipartformdata.o : src/multipartformdata.cpp $(MULITPART_DEPS)
	$(CXX) $(CXXFLAGS) -o build/multipartformdata.o -c src/multipartformdata.cpp

build/urlencoded.o : src/urlencoded.cpp $(URLENCODED_DEPS)
	$(CXX) $(CXXFLAGS) -o build/urlencoded.o -c src/urlencoded.cpp

build/runcgi.o : src/runcgi.cpp $(RUNCGI_DEPS)
	$(CXX) $(CXXFLAGS) -o build/runcgi.o -c src/runcgi.cpp

build/value.o : src/value.cpp src/value.hpp
	$(CXX) $(CXXFLAGS) -o build/value.o -c src/value.cpp

build/setter.o : src/setter.cpp src/value.hpp
	$(CXX) $(CXXFLAGS) -o build/setter.o -c src/setter.cpp

build/mustache.o : src/mustache.cpp src/mustache.hpp src/value.hpp
	$(CXX) $(CXXFLAGS) -o build/mustache.o -c src/mustache.cpp

clean :
	rm -f $(PROGRAM) $(OBJS)
