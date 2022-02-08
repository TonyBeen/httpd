CC := g++
CPP_FLAGS := -std=c++11 -Wall -g
C_FLAGS := -std=gnu99 -Wall -g

CURRENT_PATH := $(shell pwd)

INCLUDE := 	-I$(CURRENT_PATH)

STATIC_LIB := /usr/local/lib/libyaml-cpp.a /usr/local/lib/libmysqlclient.a # /usr/local/lib/libthreadpool.a
SHARED_LIB := -lhttp_parser -lcurl -lutils -llog -lpthread -ldl

SQL_DIR = $(CURRENT_PATH)/sql
NET_DIR = $(CURRENT_PATH)/net
HTTP_DIR = $(CURRENT_PATH)/http
THREAD_DIR = $(CURRENT_PATH)/thread
UTIL_DIR = $(CURRENT_PATH)/util

TEST_DIR = $(CURRENT_PATH)/test
TEST_INCLUDE_PATH = -I$(CURRENT_PATH)

testconfig_src := $(TEST_DIR)/test_config.cc config.cpp
testfiber_src := $(TEST_DIR)/test_fiber.cc fiber.cpp config.cpp
testsocket_src := $(TEST_DIR)/test_socket.cc $(NET_DIR)/socket.cpp
testaddress_src := $(TEST_DIR)/test_address.cc $(NET_DIR)/address.cpp
testhttp_src := $(TEST_DIR)/test_http.cc $(HTTP_DIR)/http.cpp
testthread_src := $(TEST_DIR)/test_thread.cc $(THREAD_DIR)/thread.cpp
testthreadpool_src := $(TEST_DIR)/test_threadpool.cc $(THREAD_DIR)/thread.cpp $(THREAD_DIR)/threadpool.cpp fiber.cpp config.cpp
testmysqlpool_src := $(TEST_DIR)/test_mysqlpool.cc $(SQL_DIR)/mysql.cpp $(SQL_DIR)/mysqlpool.cpp
testjson_src := $(TEST_DIR)/test_json.cc $(UTIL_DIR)/json.cpp $(UTIL_DIR)/cjson.cpp

default_make: httpd

test_all :
	make $(TEST_DIR)/testconfig
	make $(TEST_DIR)/testfiber
	make $(TEST_DIR)/testsocket
	make $(TEST_DIR)/testaddress
	make $(TEST_DIR)/testhttp
	make $(TEST_DIR)/testthread
	make $(TEST_DIR)/testthreadpool
	make $(TEST_DIR)/testmysqlpool
	make $(TEST_DIR)/testjson

testjson: 
	make $(TEST_DIR)/testjson

$(TEST_DIR)/testconfig : $(testconfig_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testfiber : $(testfiber_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testsocket : $(testsocket_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testaddress : $(testaddress_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testhttp : $(testhttp_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testthread : $(testthread_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testthreadpool : $(testthreadpool_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testmysqlpool : $(testmysqlpool_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)
$(TEST_DIR)/testjson : $(testjson_src)
	$(CC) $^ -o $@ $(STATIC_LIB) $(SHARED_LIB) $(CPP_FLAGS) $(TEST_INCLUDE_PATH)

.PHONY: test_all clean_test default_make

clean_test :
	-rm -rf $(TEST_DIR)/testconfig $(TEST_DIR)/testapp $(TEST_DIR)/testfiber $(TEST_DIR)/testsocket \
	$(TEST_DIR)/testaddress $(TEST_DIR)/testhttp $(TEST_DIR)/testthread $(TEST_DIR)/testthreadpool \
	$(TEST_DIR)/testjson
