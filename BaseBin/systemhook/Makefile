TARGET = systemhook.dylib
CC = clang

CFLAGS = -framework Foundation -I../.include -I./src -isysroot $(shell xcrun --sdk iphoneos --show-sdk-path) -arch arm64 -arch arm64e -miphoneos-version-min=15.0 -Wno-deprecated-declarations -Os -moutline
LDFLAGS = -dynamiclib

sign: $(TARGET)
	@strip -N $^
	@ldid -S $^

$(TARGET): $(wildcard src/*.c src/*.m ../libjailbreak/src/jbclient_xpc.c src/*.S)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	@rm -f $(TARGET)
