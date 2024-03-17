# Program details
NAME=gbust

# Paths to TMS9900 compilation tools
BASE_PATH=../../toolchain/
AS=$(BASE_PATH)tms9900/bin/tms9900-as
LD=$(BASE_PATH)tms9900/bin/tms9900-ld
CC=$(BASE_PATH)tms9900/bin/tms9900-gcc
XGA=$(BASE_PATH)xdt99/xga99.py

# List of compiled objects used in executable
# All resources and source files get added automatically. Do not fudge around with this
# To add .c or .asm files, just add them in one of the bank<?> directories
# To add source code banks, just create a new dir with the name bank<?>
# To add resources, just drop a binary file in the folder "resources" and give
# it a .dat extension. You can then find it in the ROM image at the memory location
# <filename>_bank, <filename>_offset with a length of <filename>_length
# Extra banks for data files are created as needed.
OBJECT_LIST=                                            \
  $(patsubst %.c,%.o,$(wildcard bank?/*.c))             \
  $(patsubst %.asm,%.o,$(wildcard bank?/*.asm))         \
  $(patsubst %.c,%.o,$(wildcard scratchpad/*.c))        \
  $(patsubst %.asm,%.o,$(wildcard scratchpad/*.asm))    \
  $(patsubst %.c,%.o,$(wildcard persistent/*.c))        \
  $(patsubst %.asm,%.o,$(wildcard persistent/*.asm))    \
  $(patsubst %.dat,%.o,$(wildcard resources/*.dat))

# includes and libraries
INCLUDE_PATH=../include
LIB_PATH=persistent
LIBRARIES=																							\
	-ltivgm2																							\

# List of all files needed in executable
PREREQUISITES= $(OBJECT_LIST)

# Compiler flags
CCFLAGS= $(LIBRARIES) -std=c99 -Werror -Wall -Os -s -fno-function-cse -Iinclude -I$(INCLUDE_PATH) -c

# Linker flags for flat cart binary, most of this is defined in the linker script
LDFLAGS= -L$(LIB_PATH) --no-check-sections 

# Recipe to compile the executable
all: resource_defs $(PREREQUISITES)
	@echo
	@echo "\t[XG] $(NAME)g.bin"
	@$(XGA) startgrom.gpl -o $(NAME)g.bin -B
	@echo "\t[LD] $(NAME)c.bin"
	@$(LD) --script cart.ld $(LDFLAGS) $(OBJECT_LIST) $(LIBRARIES) -o $(NAME)c.bin -M > link.map
	@echo "\t[ZP] $(NAME).rpk"
	@zip $(NAME).rpk layout.xml $(NAME)c.bin $(NAME)g.bin >> /dev/null
	@./mem_usage.sh $(NAME).bin
	@echo

dist: all
	@echo "\t[CP] Populating dist folder"
	@cp $(NAME).rpk $(NAME)c.bin $(NAME)g.bin dist/
	@echo

# Create set of resource defines for binary resources
resource_defs:
	@echo "\t[SH] Creating resource_defs.h ..."
	@./create_resourcedefs.sh include/resource_defs.h
	@echo "\t[SH] Created resource_defs.h"

# Recipe to clean all compiled objects
.phony clean:
	@echo "\tremoving compiled and intermediary files..."
	@rm -f scratchpad/*.o
	@rm -f persistent/*.o
	@rm -f bank?/*.o
	@rm -f resources/*.o
	@rm -f *.map
	@rm -f *.md5
	@rm -f $(NAME).rpk
	@rm -f $(NAME)c.bin
	@rm -f $(NAME)g.bin
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
