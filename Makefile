TARGET   = firmware
CONFIG   = config.h
SRCDIRS  = src lib/src sdk/Core sdk/Debug sdk/Peripheral/src sdk/Startup sdk/User
INCDIRS  = include lib/include sdk/Core sdk/Debug sdk/Peripheral/inc sdk/User
BUILDDIR = build
OUTPUT   = $(BUILDDIR)/$(TARGET)
LDSCRIPT = sdk/Ld/Link.ld

PREFIX   = "$(HOME)/MRS_Toolchain_Linux_X64_V240/Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf-"

OOCDDIR   = $(HOME)/MRS_Toolchain_Linux_X64_V240/OpenOCD/OpenOCD
OPENOCD   = $(OOCDDIR)/bin/openocd -f $(OOCDDIR)/bin/wch-riscv.cfg

# FLASHER   = $(OPENOCD) -c "program $(OUTPUT).elf verify" -c reset -c wlink_reset_resume -c exit
FLASHER   = $(HOME)/wlink-linux-x64/wlink flash --enable-sdi-print $(OUTPUT).elf

CC        = $(PREFIX)gcc
OBJCOPY   = $(PREFIX)objcopy
OBJDUMP   = $(PREFIX)objdump
OBJSIZE   = $(PREFIX)size
NM        = $(PREFIX)nm

# Note: using nano.specs, libc and libm are replaced with their nano variants
# 		nosys.specs provides stubs for standard syscalls
# 		also note that libg is the debug build of libc
CFLAGS    = -march=rv32ec_xw -mabi=ilp32e -g -nostdlib -specs=nano.specs -specs=nosys.specs
CFLAGS   += -flto -ffunction-sections -fdata-sections -fno-builtin
CFLAGS   += -include $(CONFIG) $(addprefix -I,$(INCDIRS))
CFLAGS   += -Wall -Wno-misleading-indentation
DEPFLAGS  = -MMD -MP
LDFLAGS   = -T$(LDSCRIPT) -Wl,--gc-sections -lc -lm -lgcc

SOURCES  := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
SOURCES  += $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.S))
OBJECTS  := $(addprefix $(BUILDDIR)/,$(SOURCES))
OBJECTS  := $(OBJECTS:%.c=%.o)
OBJECTS  := $(OBJECTS:%.S=%.o)
DEPFILES := $(OBJECTS:%.o=%.d)

all: bin asm lst map size

$(DEPFILES):
-include $(DEPFILES)

$(BUILDDIR)/%.o: %.c $(BUILDDIR)/%.d
	@echo "Compiling $<"
	@mkdir -p $(@D)
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S $(BUILDDIR)/%.d
	@echo "Assembling $<"
	@mkdir -p $(@D)
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

$(OUTPUT).elf: $(OBJECTS)
	@echo "Linking $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin: $(OUTPUT).bin
%.bin: %.elf
	@$(OBJCOPY) -O binary $< $@

asm: $(OUTPUT).asm
%.asm: %.elf
	@$(OBJDUMP) -d $< > $@

lst: $(OUTPUT).lst
%.lst: %.elf
	@$(OBJDUMP) -S $< > $@

map: $(OUTPUT).map
%.map: %.elf
	@$(OBJDUMP) -t $< > $@

size: $(OUTPUT).elf
	@$(OBJSIZE) -d $<

symbols: $(OUTPUT).elf
	@$(NM) -S --size-sort $<

debug: $(OUTPUT).elf
	@$(OPENOCD)

flash: $(OUTPUT).bin
	@$(FLASHER)

clean:
	@rm -rf $(BUILDDIR)

.PHONY: all bin asm lst map size symbols debug flash clean
.SUFFIXES:
