#include "bankswitch.h"

# TI99/4A cart header. 
# This needs to be put at 0x6000, so the linker needs to target this file specifically

# GROM header
  byte 0xAA, 0x01, 0x00, 0x00

# Start of power-up chain (unused)
  data 0x0000

# Start of program chain
  data program_record

# Start of DSR chain (unused)
  data 0x0000 

# Start of subprogram list (unused)
# This doubles as the terminator of the program chain
program_record:
  data  0x0000          # Next program chain record
  data  _start_bank     # Entry point for program
  nstring "GHOSTBUSTERS"# Name of program
  even

# Jump to bank 0
_start_bank             # Entry point for program
  li  r9, >ASM_ADDRESS  # Load bank 0 address in register
  clr *r9               # Write to memory address in r9 (any write will do, clr is quick)
  b   @_start           # Branch to real program entry point: _start function in crt0.c
