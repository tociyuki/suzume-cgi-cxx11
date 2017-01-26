PROGRAM=suzume.cgi
OBJS=build/main.o build/encode-utf8.o build/mustache.o \
     build/multipartformdata.o build/content-length.o \
     build/urlencoded.o build/runcgi.o

MAIN_DEPS=src/sqlite3pp.hpp src/mustache.hpp \
	 src/encode-utf8.hpp src/http.hpp \
	 src/suzume_data.hpp src/suzume_view.hpp
ENCODEUTF8_DEPS=src/encode-utf8.hpp
MUSTACHE_DEPS=src/mustache.hpp
CONTENTLEN_DEPS=src/http.hpp
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
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	chmod 755 $(PROGRAM)

mustache-test : build/mustache-test.o build/mustache.o
	$(CXX) $(CXXFLAGS) build/mustache-test.o build/mustache.o -o $@

build/mustache-test.o : src/mustache-test.cpp
	$(CXX) $(CXXFLAGS) -c src/mustache-test.cpp -o $@

build/main.o : src/main.cpp $(MAIN_DEPS)
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o $@

build/encode-utf8.o : src/encode-utf8.cpp $(ENCODEU8_DEPS)
	$(CXX) $(CXXFLAGS) -c src/encode-utf8.cpp -o $@

build/mustache.o : src/mustache.cpp $(MUSTACHE_DEPS)
	$(CXX) $(CXXFLAGS) -c src/mustache.cpp -o $@

build/content-length.o : src/content-length.cpp $(CONTENTLEN_DEPS)
	$(CXX) $(CXXFLAGS) -c src/content-length.cpp -o $@

build/multipartformdata.o : src/multipartformdata.cpp $(MULITPART_DEPS)
	$(CXX) $(CXXFLAGS) -c src/multipartformdata.cpp -o $@

build/urlencoded.o : src/urlencoded.cpp $(URLENCODED_DEPS)
	$(CXX) $(CXXFLAGS) -c src/urlencoded.cpp -o $@

build/runcgi.o : src/runcgi.cpp $(RUNCGI_DEPS)
	$(CXX) $(CXXFLAGS) -c src/runcgi.cpp -o $@

clean :
	rm -f $(PROGRAM) $(OBJS) mustache-test
