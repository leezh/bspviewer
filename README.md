# BSPViewer

This is a simple program that displays Quake 3 BSP maps.

Almost everything here was based off the file format specifications found online [here](http://www.mralligator.com/q3/) and [here](http://graphics.cs.brown.edu/games/quake/quake3.html) rather than the source code.

## Building and Installing

Ensure that the following packages and their development files are installed properly:

  * GNU GCC C++
  * GNU Make
  * CMake
  * SFML
  * PhysicsFS

On Ubuntu they can all be installed using:

    sudo apt install g++ cmake libsfml-dev libphysfs-dev

To build use the following commands:

    mkdir build
    cd build
    cmake ..
    make

Alternatively, you could use QtCreator to open up CMakeLists.txt to compile the code.

## Usage

To use you will need an install of Quake 3 and the location of its data folder `q3base`. On Steam this can typically be found in `C:\Program Files\Steam\steamapps\common\Quake 3 Arena\baseq3\`.

A list of availible maps can be shown using: `bspviewer /path/to/baseq3/`

To load a map use: `bspviewer /path/to/baseq3/ /maps/q3ctf1.bsp`

  * Mouse movement for looking
  * WASD for directional movement
  * Space to move up
  * Shift to move down
  * Escape to quit

## License

BSPViewer

Copyright (C) 2010-2016 Lee Zher Huei (leezh@leezh.me)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.

