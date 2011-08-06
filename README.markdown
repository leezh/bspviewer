# BSPViewer
This is a simple program that displays Quake 3 BSP maps. It uses only C++ and Qt so it should theoretically work on any device that supports QtOpenGL.

Almost everything here was based off the file format specifications found online [here](http://www.mralligator.com/q3/) and [here](http://graphics.cs.brown.edu/games/quake/quake3.html) rather than the source code.

## Building and Installing

Ensure that the following packages and their development files are installed properly:

  * GNU GCC C++
  * GNU Make
  * CMake
  * Qt

Run the following commands:

    cmake .
    make
    make install

Alternatively, you could use QtCreator to open up CMakeLists.txt to compile the code.

## Usage
To avoid legal issues, no map files or textures are included. If you don't already own Quake3, there are custom maps available online that don't require the official files.

To prepare, copy the texture folder and the `.bsp` file into the project directory. Rename the map file into map.bsp.

  * Mouse movement for looking
  * WASD/Arrow-keys for directional movement
  * Space/PageUp to move up
  * Shift/PageDown to move down
  
## ToDo
  * Shader script support
  * Light volumes (buggy, disabled by default)
  * Dynamic lighting
  * Various optimisations
  * Collision detection (partial)

## License

    BSPViewer
    Copyright (C) 2010-2011 Lee Zher Huei <lee.zh.92@gmail.com>
    
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

