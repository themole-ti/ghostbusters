# Program details
NAME=gbust

# Paths to TMS9900 compilation tools
BASE_PATH=../../toolchain/
AS=$(BASE_PATH)tms9900/bin/tms9900-as
LD=$(BASE_PATH)tms9900/bin/tms9900-ld
CC=$(BASE_PATH)tms9900/bin/tms9900-gcc
XGA=$(BASE_PATH)xdt99/xga99.py

# includes and libraries
INCLUDE_PATH=../include				# The path to libti99 header files
LIB_PATH=persistent
LIBRARIES=																							\
	-ltivgm2																							\

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

# List of all files needed in executable
PREREQUISITES= $(OBJECT_LIST)

# Compiler flags
CCFLAGS= $(LIBRARIES) -std=c99 -Werror -Wall -Os -s -fno-function-cse -Iinclude -I$(INCLUDE_PATH) -c

# Linker flags for flat cart binary, most of this is defined in the linker script
LDFLAGS= -L$(LIB_PATH) --no-check-sections 

# Recipe to compile the executable
all: check_dependencies resource_defs $(PREREQUISITES)
	@echo
	@echo "\t[XG] $(NAME)g.bin"
	@$(XGA) startgrom.gpl -o $(NAME)g.bin -B --color on
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

# Check if dependencies can be found
check_dependencies:
ifeq (,$(wildcard $(XGA)))
	@echo "ERROR - xga99.py not found!"
	@echo
	@echo "Please make sure you've installed the tms9900 gcc port."
	@echo "Download and install the tools from here: https://github.com/endlos99/xdt99"
	@echo "and make sure the XGA path at the top of this Makefile is defined correctly\n\t (\$$BASE_PATH=$(BASE_PATH))\n\t (\$$XGA=$(XGA))"
	@echo
	@exit 1
endif
ifeq (,$(wildcard $(CC)))
	@echo "ERROR - tms9900-gcc not found!"
	@echo
	@echo "Please make sure you've installed the xdt99 cross development tools."
	@echo "Download and install the compiler from here: https://github.com/mburkley/tms9900-gcc"
	@echo "and make sure the toolchain path at the top of this Makefile is defined correctly\n\t (\$$BASE_PATH=$(BASE_PATH))\n\t (\$$CC=$(CC))"
	@echo
	@exit 1
endif
ifeq (,$(wildcard $(INCLUDE_PATH)/TISNPlay.h))
	@echo "ERROR - TISNPlay.h not found!"
	@echo
	@echo "Please install Tursi's libti99 library first."
	@echo "Download and install the libti99 project from here: https://github.com/tursilion/libti99"
	@echo "and make sure the include files are found in the include path defined in this Makefile\n\t (\$$INCLUDE_PATH=$(INCLUDE_PATH))"
	@echo
	@exit 1
endif
ifeq (,$(wildcard ./persistent/libtivgm2.a))
	@echo "ERROR - libtivgm2.a not found!"
	@echo
	@echo "Please install Tursi's vgmcomp2 player first."
	@echo "Download libtivgm2.a from here: https://github.com/tursilion/vgmcomp2/blob/48c51fe354475b0e3085dd125bedd65ec59dc107/Players/libtivgm2/libtivgm2.a"
	@echo "and copy it into the 'persistent' directory."
	@echo
	@exit 1
endif

# Create set of resource defines for binary resources
resource_defs:
	@echo "\t[SH] Creating resource_defs.h ..."
	@./create_resourcedefs.sh include/resource_defs.h

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
