# Program details
NAME=ghostbusters
INSTALL_DIR=~/mame0202-64bit/carts
RESOURCE_DIR=./resources

# Paths to TMS9900 compilation tools
BASE_PATH=../../toolchain/tms9900
AS=$(BASE_PATH)/bin/tms9900-as
LD=$(BASE_PATH)/bin/tms9900-ld
CC=$(BASE_PATH)/bin/tms9900-gcc
INCLUDE_PATH=../include
LIB_PATH=persistent


# List of compiled objects used in executable
# All resources and source files get added automatically. Do not fudge around with this
# To add .c or .asm files, just add them in one of the bank<?> directories
# To add source code banks, just create a new dir with the name bank<?>
# To add resources, just drop a binary file in the folder "resources" and give
# it a .dat extension. You can then find it in the ROM image between memory locations
# _binary_resources_<filename>_dat_start and _binary_resources_<filename>_dat_end
# Extra banks for data files are created as needed.
OBJECT_LIST=                                            \
  $(patsubst %.c,%.o,$(wildcard bank?/*.c))             \
  $(patsubst %.asm,%.o,$(wildcard bank?/*.asm))         \
  $(patsubst %.c,%.o,$(wildcard scratchpad/*.c))        \
  $(patsubst %.asm,%.o,$(wildcard scratchpad/*.asm))    \
  $(patsubst %.c,%.o,$(wildcard persistent/*.c))        \
  $(patsubst %.asm,%.o,$(wildcard persistent/*.asm))    \
  $(patsubst %.dat,%.o,$(wildcard resources/*.dat))

LIBRARIES=																							\
	-ltivgm2																							\

# List of all files needed in executable
PREREQUISITES= $(OBJECT_LIST)

# Compiler flags
CCFLAGS= $(LIBRARIES) -std=c99 -Werror -Wall -Os -s -Iinclude -I$(INCLUDE_PATH) -c

# Linker flags for flat cart binary
# Most of this is defined in the linker script
LDFLAGS= -L$(LIB_PATH)

# Recipe to compile the executable
all: resource_defs $(PREREQUISITES)
	@echo
	@echo "\t[LD] $(NAME).bin"
	@$(LD) --script cart.ld $(LDFLAGS) $(OBJECT_LIST) $(LIBRARIES) -o $(NAME).bin -M > link.map
	@echo "\t[ZP] $(NAME).rpk"
	@zip $(NAME).rpk layout.xml $(NAME).bin >> /dev/null
	@./mem_usage.sh

geneve: resource_defs_geneve $(PREREQUISITES)
	@echo
	@echo "\t[LD] $(OBJECT_LIST) $(LIBRARIES) -> $(NAME).bin"
	@$(LD) --script geneve.ld $(LDFLAGS) $(OBJECT_LIST) $(LIBRARIES) -o $(NAME).bin -M > link.geneve.map
	@echo

# Create set of resource defines for binary resources
resource_defs:
	@echo "\t[SH] Creating resource_defs.h"
	@./create_resourcedefs.sh include/resource_defs.h

# Create set of resource defines for binary resources
resource_defs_geneve:
	@echo "\t[SH] Creating resource_defs.h"
	@./create_resourcedefs.sh include/resource_defs.h geneve

# Build GPU program
#gpu:
#	$(CC) -std=c99 -Werror -Wall -c gpucode.c -Os -s -o gpucode.o
#	$(LD) gpucode.o --section-start .text=0x1900 -o gpucode.elf
#	$(OBJCPY) -O binary gpucode.elf gpucode.bin
#	bin2h gpucode.bin -b gpucode -f gpucode.h

# Recipe to clean all compiled objects
.phony clean:
	@echo "\tremoving compiled and intermediary files..."
	@rm -f scratchpad/*.o
	@rm -f persistent/*.o
	@rm -f bank?/*.o
	@rm -f resources/*.o
	@rm -f $(NAME).rpk
	@rm -f link.map
	@rm -f $(NAME).bin
	@echo


# Recipe to convert binary files to elf objects
# for inclusion by the linker
%.o: %.dat
	@echo "\t[LD] $<..."
	@$(LD) -r -b binary $< -o $@

# Recipe to compile all assembly files
%.o: %.asm
	@echo "\t[AS] $<..."
	@$(AS) $< -o $@

# Recipe to compile all C files
%.o: %.c
	@echo "\t[CC] $<..."
	@$(CC) $(CCFLAGS) $< -o $@
