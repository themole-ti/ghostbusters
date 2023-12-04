# ghostbusters
Remake of Activision's 1984 Ghostbusters game, drawing inspiration from the C64 and Sega Master System versions. Built from scratch by playing the originals way too many times and trying to recreate the original game logic. Graphic style is inspired by the Master System version, but the game logic is based entirely on the C64 original.

# Building
You need the gcc port for the tms9900 CPU installed on your system and available in your path. You can download the compiler from [here](https://github.com/mburkley/tms9900-gcc). Note that my customer build 'system' uses a few shell scripts and has only been tested on Linux and macos. Building on Windows will probably require WSL and hasn't been tested.
Once you have the compiler installed and ready to go, simply issue a `make` command in the root folder of the project, and it should build two files called `ghostbusters.bin` and `ghostbusters.rpk`.

# Playing 
To use the binaries with Classic99, rename `ghostbusters.bin` to `ghostbusters8.bin`, for Mame use `ghostbusters.rpk`, js99er.net works fine with both files.
If you want to play the game on real hardware, you will need to either burn the binary to a 512kb rom cartridge of the non-inverting type, or use a flash cart like the FinalGrom99 or the BackBit cartridge.

Note that the game requires the 32kb memory expansion to be installed.
