TARGET = kernel.elf
TARGET_DIR = ~/workspace/MikanOS_X/bin
OBJS = main.o
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17
LDFLAGS += --entry KernelMain -z norelro --image-base 0x100000 --static

all:$(TARGET)

clean:
	rm -rf *.o

$(TARGET): $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET_DIR)/kernel.elf $(OBJS)

%.o:%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

.PHONY: all clean