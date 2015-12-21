#!/bin/sh

echo extracting all sprites from Room1.Bobs
../amosbank Room1.Bobs
identify -quiet -format "%#\n" Room1.Bobs.*.png | sort > Bobs.signature

echo extracting Pac.Pic. compressed image from Room1.Pic, using the palette from Room1.Bobs
../amosbank Room1.Pic Room1.Bobs
identify -quiet -format "%#\n" Room1.Pic.png | sort > Pic.signature

# compare visual signatures
for file in Bobs.signature Pic.signature
do
  diff $file ${file}.gold > /dev/null
  if [ $? -ne 0 ]
  then
    echo $file failed
  else
    echo $file passed
  fi
done
