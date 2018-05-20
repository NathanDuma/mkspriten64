# mksprite64

Windows equivalent so the SGI program mksprite for the Nintendo 64. Converts png to a h header and c source file.

Usage:
Specify file path (must end in .png): -f file.png

OPTIONAL:

Scale in x direction: -sx scaleX

Scale in y direction: -sy scaleY

Scale is 1.0 by default.


Show preview in c file: -p t/f

Preview is false by default.

Colour mode: -m 16/32

Colour mode is 16-bit RGBA by default


Please note that if you run multiple instances of mkspriten64 at once, it would be wise to have a delay between runs since they will all try to write to the same file, common_sprites.h. This file just makes it convienent to include all sprites in one file and so this is optional.
