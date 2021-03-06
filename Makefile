OBJS=cJSON.o test.o
CXXFLAGS=-std=c++0x -O2 -Wall -Wextra -pedantic-errors
CFLAGS=-std=c99 -O2 -Wall -Wextra -pedantic-errors
LDFLAGS=-g3

CXX_R=@echo "   CXX" $@;$(CXX)
CC_R=@echo "   CC" $@;$(CC)
LD_R=@echo "   LD" $@;$(CXX)

all: testcjsonpp

clean:
	$(RM) testcjsonpp test.o cJSON.o

testcjsonpp: $(OBJS)
	$(LD_R) -o $@ $(LDFLAGS) $(OBJS)

test.o: test.cc cjsonpp.h
	$(CXX_R) -o $@ -c $(CXXFLAGS) $<

cJSON.o: cJSON.c
	$(CC_R) -o $@ -c $(CFLAGS) $<
