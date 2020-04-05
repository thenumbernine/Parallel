PARALLEL_PATH:=$(dir $(lastword $(MAKEFILE_LIST)))
INCLUDE+=$(PARALLEL_PATH)include
CFLAGS_linux+=-pthread
LDFLAGS_linux+=-pthread
