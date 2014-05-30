DIST_FILENAME=test
DIST_TYPE=app

include ../../GLApp/Makefile.mk

INCLUDE=../include ../../GLApp/include
LIBPATHS=../dist/$(PLATFORM)/$(BUILD)
LIBS=Parallel

$(DIST):: $(OBJPATHS)
	install_name_tool -change dist/$(PLATFORM)/$(BUILD)/libGLApp.dylib ../../GLApp/dist/$(PLATFORM)/$(BUILD)/libGLApp.dylib $@
	install_name_tool -change dist/$(PLATFORM)/$(BUILD)/libParallel.dylib ../dist/$(PLATFORM)/$(BUILD)/libParallel.dylib $@

