PARALLEL_PATH:=$(dir $(lastword $(MAKEFILE_LIST)))

INCLUDE+=$(PARALLEL_PATH)include
DYNAMIC_LIBS+=$(PARALLEL_PATH)dist/$(PLATFORM)/$(BUILD)/libParallel.dylib

