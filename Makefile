include define.mk

CPPSRC := $(wildcard $(CURRENT_PATH)/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/util/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/net/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/http/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/thread/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/sql/*.cpp)
CPPSRC += $(wildcard $(CURRENT_PATH)/util/*.cpp)

CSRC += $(wildcard $(CURRENT_PATH)/util/*.c)

CPPOBJ := $(patsubst %.cpp, %.o, $(CPPSRC))
COBJ := $(patsubst %.c, %.o, $(CSRC))

httpd : $(CPPOBJ)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB)
	-rm -rf $(CPPOBJ)

%.o : %.cpp
	$(CC) $^ -c -o $@ $(CPP_FLAGS) $(INCLUDE)
%.o : %.c
	g++ $^ -c -o $@ $(C_FLAGS) $(INCLUDE)

.PHONY : httpd clean

clean :
	-rm -rf $(CPPOBJ)