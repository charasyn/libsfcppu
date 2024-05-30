name := libsfcppu
build := performance
openmp := true
flags += -I.

nall.path := nall
include $(nall.path)/GNUmakefile

ifeq ($(platform), ios-arm64)
	flags += -fPIC -miphoneos-version-min=11.0 -Wno-error=implicit-function-declaration -DHAVE_POSIX_MEMALIGN
	options += -dynamiclib
else ifeq ($(platform), tvos-arm64)
	flags += -fPIC -mtvos-version-min=11.0 -Wno-error=implicit-function-declaration -DHAVE_POSIX_MEMALIGN
	options += -dynamiclib
endif

dirs := obj out
$(dirs):
	mkdir $@

objects := obj/$(name).o

obj/$(name).o: $(name).cpp | $(dirs)
obj/test.o: test.cpp
	$(call compile,$(shell pkg-config --cflags SDL2))

all: $(objects)
ifeq ($(platform),linux)
	$(strip $(compiler) -o out/$(name).so -shared $(objects) -Wl,--no-undefined -Wl,--version-script=link.T -lgomp -Wl,-Bdynamic $(options))
else ifeq ($(platform),windows)
	$(strip $(compiler) -o out/$(name).dll -shared $(objects) -Wl,--no-undefined -Wl,--version-script=link.T -lgomp -Wl,-Bdynamic $(options))
else ifeq ($(platform),macos)
	$(strip $(compiler) -o out/$(name).dylib -shared $(objects) $(options))
else ifeq ($(platform), ios-arm64)
    ifeq ($(IOSSDK),)
       IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
    endif
	$(strip c++ -arch arm64 -marm -miphoneos-version-min=11.0 -isysroot $(IOSSDK) -o out/$(name)_ios.dylib -shared $(objects) -lpthread -ldl)
else ifeq ($(platform), tvos-arm64)
    ifeq ($(IOSSDK),)
       IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
    endif
	$(strip c++ -arch arm64 -marm -mtvos-version-min=11.0 -isysroot $(IOSSDK) -o out/$(name)_tvos.dylib -shared $(objects) -lpthread -ldl)
endif

test: out/test
out/test: obj/test.o $(objects)
	$(compiler) -o $@ $^ $(shell pkg-config --libs --cflags SDL2)

.PHONY: all clean
clean:
	$(call delete,obj/*)
	$(call delete,out/*)
