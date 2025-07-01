CFLAGS = -I"$(VULKAN_SDK)" -I"lib\glfw-3.4\include"
LDFLAGS = -L"$(VULKAN_SDK)\Lib" -L"lib\glfw-3.4\lib-vc2022" -lvulkan-1 -lglfw3 -luser32 -lgdi32 -lshell32 -lvcruntime -lmsvcrt -llibcmt

SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))

all: build app

build:
	mkdir -p build

build/%.o: src/%.c
	clang $(CFLAGS) -c $< -o $@

app: $(OBJECTS)
	clang $^ $(LDFLAGS) -o build/vksnd.exe
