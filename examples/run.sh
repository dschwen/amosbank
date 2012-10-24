#!/bin/sh

echo extracting all sprites from Room1.Bobs
./amosbank Room1.Bobs

echo extracting Pac.Pic. compressed image from Room1.Pic, using the palette from Room1.Bobs
./amosbank Room1.Pic Room1.Bobs

