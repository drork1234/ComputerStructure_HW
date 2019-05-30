# This is the Makefile
CXX=g++
CXXFLAGS=-g -Wall -std=c++0x
CXXLINK=$(CXX)
LIBS=
OBJS=main.o L1Cache.o L2Cache.o CacheSim.o CacheLine.o Cache.o CacheBlock.o
RM=rm -f

cacheSim: $(OBJS)
	$(CXXLINK) -o cacheSim $(OBJS) $(LIBS)

CacheBlock.o: CacheBlock.cpp CacheBlock.h
Cache.o: Cache.cpp Cache.h CacheLine.h CacheBlock.h
CacheLine.o: CacheLine.cpp CacheLine.h CacheBlock.h
CacheSim.o: CacheSim.cpp CacheSim.h Cache.h CacheLine.h CacheBlock.h
L1Cache.o: L1Cache.cpp Cache.h CacheLine.h CacheBlock.h
L2Cache.o: L2Cache.cpp Cache.h CacheLine.h CacheBlock.h
main.o: main.cpp CacheSim.h Cache.h CacheLine.h CacheBlock.h

clean:
	$(RM) cacheSim *.o *.bak *~ "#"* core
