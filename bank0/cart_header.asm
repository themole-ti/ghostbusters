#include "bankswitch.h"

# TI99/4A cart header. 
# This needs to be put at 0x6000, so the linker needs to target this file specifically

# GROM header
  byte 0xAA, 0x01, 0x00, 0x00

# chain pointers
  data 0x0000             # Start of power-up chain (unused)
  data program_record     # Start of program chain
  data 0x0000             # Start of DSR chain (unused)
  data 0x0000             # Start of subprogram chain (unused)
  data 0x0000             # Start of interrupt chain (unused)
  data 0x0000             # unused

  data _vector            # make _vector available from XML jump table (0x70 -> ROM location 0x6010)

# Program chain
program_record:
  data  0x0000            # Next program chain record
  data  _vector           # Entry point for program
  nstring "GHOSTBUSTERS"  # Name of program
  even

# Start our program
_vector                   # Entry point for program
  b   @_start             # Branch to real program entry point: _start function in crt0.c
