include ./maketest.mk

CPPSRC := $(wildcard $(CURRENT_PATH)/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/util/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/net/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/http/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/thread/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/sql/*.cpp)

OBJ := $(patsubst %.cpp, %.o, $(CPPSRC))

httpd : $(OBJ)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB)
	-rm -rf $(OBJ)

%.o : %.cpp
	$(CC) $^ -c -o $@ $(CPP_FLAGS) $(INCLUDE)

.PHONY : httpd clean

clean :
	-rm -rf $(OBJ)