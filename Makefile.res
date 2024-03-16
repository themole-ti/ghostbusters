# Paths to TMS9900 compilation tools
BASE_PATH=../../toolchain/tms9900/bin
LD=$(BASE_PATH)/tms9900-ld

# List of compiled objects used in executable
# All resources get added automatically by the last line. Do not replace
# To add resources, just drop a binary file in the folder "resources" and give
# it a .dat extension. You can then find it in the ROM image between locations
# _binary_resources_<filename>_dat_start and _binary_resources_<filename>_dat_end 
OBJECT_LIST= 					\
  $(patsubst %.dat,%.o,$(wildcard resources/*.dat))

# List of all files needed in executable
PREREQUISITES= $(OBJECT_LIST)

# Recipe to compile the executable
all: $(PREREQUISITES)
	@$(LD) --script bin.ld $(OBJECT_LIST) -o temp.bin -M > res.map
	@rm temp.bin

# Recipe to convert binary files to elf objects
# for inclusion by the linker
%.o: %.dat
	@echo "\t[LD] $<..."
	@$(LD) -r -b binary $< -o $@
