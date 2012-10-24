amosbank
========

A converter/unpacker for AMOS banks (the Amiga programming language)

Usage: amosbank bankfile \[palettebank\]

bankfile is the AMOS bank to be extacted. The optional palettebank is the filename of an icon or sprite bank that can provide a color palette if bankfile is a Pac.Pic. file.


This project is in its infancy. Current features are:

* writing all sprites/icons as individual PNG files
* listing of sprite/icon bank color palettes
* listing of sprite/icon bank sprites (dimensions, hotspots)

* writing Pac.Pic. picture as PNG file
* allow fetching palette data from external icon or sprite file
* listing of Pac.Pic. picture dimensions

* listing of bank names

Planned features:
* write a JSON file with sprite/icon hotspots


File format documentation:
* [Generic Banks](http://www.exotica.org.uk/wiki/AMOS_file_formats)
* [Pac.Pic.](http://www.exotica.org.uk/wiki/AMOS_Pac.Pic._format)

