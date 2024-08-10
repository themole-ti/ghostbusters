# ghostbusters
Remake of Activision's 1984 Ghostbusters game, drawing inspiration from the C64 and Sega Master System versions. Built from scratch by playing the originals way too many times and trying to recreate the original game logic. Graphic style is mostly inspired by the Master System version, but the game logic is based entirely on the C64 original.

# Building
Binaries for emulators and flash carts are provided in the dist folder, so most people will not need to worry about this section. If you do want build from source, the below should get you started.

You need the gcc port for the tms9900 CPU installed on your system and available in your path. You can download the compiler from [here](https://github.com/mburkley/tms9900-gcc). Note that my custom build 'system' uses a few shell scripts and has only been tested on Linux and macos. Building on Windows will probably require WSL and hasn't been tested.

In order to build the "boot grom", you need the xga99 GPL assembler from the xdt99 development tools created by Ralph Benzinger, which can be found [here](https://github.com/endlos99/xdt99)

The game also uses Tursi's libti99 for certain functions (mostly the sound player), so make sure you have that installed in your include and library paths.

Once you have the compiler and GPL assembler installed and ready to go, check the Makefiles (`Makefile` and `Makefile.res`) and make sure that the paths point to your toolchain binaries. Then, simply issue a `make` command in the root folder of the project, and it should start the build process.
When the build has completed, you will find two files called `gbustc.bin` and `gbustg.bin`.  In addition to this, a `gbust.rpk` file is created for use with Mame and js99er.net.

# Playing 
To use the binaries with Classic99, load `gbustc.bin`, for Mame use `gbust.rpk`. For js99er.net, load either *both* `gbustc.bin` *and* `gbustg.bin`, or simply use the rpk file.
If you want to play the game on real hardware, you will need to either burn the binaries to an Ubergrom style cart with 512kb of ROM and GROM emulation, or use a flash cart like the FinalGrom99 or the BackBit cartridge. Naming conventions and requirements might differ from flash cart to flash cart, so check your cart's documentation to know if you need to rename the files.
Note that on flash carts, the game will likely show up as an empty menu item (option A. on page 1 on the FinalGrom99). This is expected behavior due to the way the game 'hacks' the TI's start-up procedure.

Also note that the game requires the 32kb memory expansion to run.

# Acknowledgements
The libtivgm2.a music player was created by Tursi and the binary is distributed with his permission. The original can be found [here](https://github.com/tursilion/vgmcomp2)
