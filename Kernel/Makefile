TARGET = kernel.elf
TARGET_DIR = ~/workspace/MikanOS_X/bin
OBJS = main.o graphics.o font.o newlib_support.o console.o
DEPENDS = $(join $(dir $(OBJS)), $(addprefix .,$(notdir $(OBJS:.o=.d))))

CFLAGS 		+= -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS 	+= -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17
LDFLAGS += --entry KernelMain -z norelro --image-base 0x100000 --static

all:$(TARGET)

clean:
	rm -rf *.o

$(TARGET): $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET_DIR)/kernel.elf $(OBJS) ~/workspace/MikanOS_X/resource/hankaku.o -lc

%.o:%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.bin
	touch $@

.PHONY: all clean depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)