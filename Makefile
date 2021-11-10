include ./maketest.mk

CPPSRC := $(wildcard ./*.cpp)
CPPSRC += $(wildcard ./util/*.cpp)
CPPSRC += $(wildcard ./net/*.cpp)
CPPSRC += $(wildcard ./http/*.cpp)
CPPSRC += $(wildcard ./thread/*.cpp)

OBJ := $(patsubst %.cpp, %.o, $(CPPSRC))

httpd : $(OBJ)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB)
	-rm -rf $(OBJ)

%.o : %.cpp
	$(CC) $^ -c -o $@ $(CPP_FLAGS) $(INCLUDE)

.PHONY : httpd clean

clean :
	-rm -rf $(OBJ)