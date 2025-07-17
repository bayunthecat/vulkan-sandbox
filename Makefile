CFLAGS = -I"$(VULKAN_SDK)\include" -I"include"
LDFLAGS = -L"$(VULKAN_SDK)\Lib" -Llib -lvulkan-1 -lglfw3dll -lcglm -MD

SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))

all: build app

build:
	mkdir build

build/%.o: src/%.c
	clang -v $(CFLAGS) -c $< -o $@ 

app: $(OBJECTS)
	clang -v $^ $(LDFLAGS) -o build/vksnd.exe
