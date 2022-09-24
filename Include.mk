PARALLEL_PATH:=$(dir $(lastword $(MAKEFILE_LIST)))
INCLUDE+=$(PARALLEL_PATH)include
CXXFLAGS_linux+=-pthread
LDFLAGS_linux+=-pthread
