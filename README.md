amosbank
========

A converter/unpacker for AMOS banks (the Amiga programming language)

(c) by Daniel Schwen, 2012

Licensed under CC-BY-SA 3.0


Usage:

    amosbank bankfile \[palettebank\]

bankfile is the AMOS bank to be extacted. The optional palettebank is the filename of an icon or sprite bank that can provide a color palette if bankfile is a Pac.Pic. file.


This project is in its infancy. Current features are:

* **writing all sprites/icons as individual PNG files**
* listing of sprite/icon bank color palettes
* listing of sprite/icon bank sprites (dimensions, hotspots)
* **writing Pac.Pic. picture as PNG file**
* allow fetching palette data from external icon or sprite file
* listing of Pac.Pic. picture dimensions
* listing of bank names
* support for Extra Half-Brite and HAM images!
* **writing sample banks as individual WAV files**
* **writing certain memory banks as JSON**

Planned features:
* write a JSON file with sprite/icon hotspots
* AMAL banks

File format documentation:
* [Generic Banks](http://www.exotica.org.uk/wiki/AMOS_file_formats)
* [Pac.Pic.](http://www.exotica.org.uk/wiki/AMOS_Pac.Pic._format)

HAM images are saved as 24bit RGB PNG files. All other image formats are saved as indexed color images. This may allow recovering palette modification effects that were frequently used on the Amiga. For Icons and Sprites color zero is given an alpha value of 0.0, making color 0 fully transparent. Opacity and the original color is recoverable by setting the alpha value back to 1.0 in an image editor. In image files all colors are fully opaque.

Samples are assumed to be always mono 8bit. They are converted from signed 8bit to unsigned 8bit (what is what PCM expects) by adding 128 to each sample value.

To convert AMOS Music Banks use [Abk2Mod-II](http://aminet.net/package/dev/amos/Abk2Mod-II) on AmigaOS (emulator). The resulting protracker module can be converted to mp3 using xmp

    xmp -o file.wav -b 16 file.mod
    lame -V2 file.wav file.mp3
