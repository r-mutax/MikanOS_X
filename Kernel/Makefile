TARGET = kernel.elf
TARGET_DIR = ~/workspace/MikanOS_X/bin
OBJS = 	main.o \
		graphics.o \
		font.o \
		newlib_support.o \
		console.o \
		pci.o \
		asmfunc.o \
		logger.o \
		mouse.o \
		libcxx_support.o \
		interrupt.o \
		segment.o \
		paging.o \
		memory_manager.o \
		window.o \
		layer.o \
		timer.o \
		acpi.o \
		keyboard.o \
		task.o \
		frame_buffer.o \
		terminal.o \
		fat.o \
		syscall.o \
		usb/memory.o \
		usb/device.o \
		usb/xhci/ring.o \
		usb/xhci/trb.o \
		usb/xhci/xhci.o \
        usb/xhci/port.o \
		usb/xhci/device.o \
		usb/xhci/devmgr.o \
		usb/xhci/registers.o \
        usb/classdriver/base.o \
		usb/classdriver/hid.o \
		usb/classdriver/keyboard.o \
        usb/classdriver/mouse.o
DEPENDS = $(join $(dir $(OBJS)), $(addprefix .,$(notdir $(OBJS:.o=.d))))

CPPFLAGS += -I.
CFLAGS 		+= -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS 	+= -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
               -fno-exceptions -fno-rtti -std=c++17
LDFLAGS += --entry KernelMain -z norelro --image-base 0x100000 --static

all:$(TARGET)

clean:
	rm -rf *.o
	rm -rf usb/*.o
	rm -rf usb/classdriver/*.o
	rm -rf usb/xhci/*.o
	rm -rf .*.d
	rm -rf usb/.*.d
	rm -rf usb/classdriver/.*.d
	rm -rf usb/xhci/.*.d

$(TARGET): $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET_DIR)/kernel.elf $(OBJS) ~/workspace/MikanOS_X/resource/hankaku.o -lc -lc++ -lc++abi -lm -lfreetype

%.o:%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

.%.d: %.bin
	touch $@

.PHONY: all clean depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)