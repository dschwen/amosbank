#!/bin/bash

echo extracting all sprites from Room1.Bobs
../amosbank Room1.Bobs
identify -quiet -format "%#" `ls Room1.Bobs.?.png; ls Room1.Bobs.??.png` | tr -d '\n' > Bobs.signature

echo extracting Pac.Pic. compressed image from Room1.Pic, using the palette from Room1.Bobs
../amosbank Room1.Pic Room1.Bobs
identify -quiet -format "%#" Room1.Pic.png | tr -d '\n' > Pic.signature

# compare visual signatures
fails=0
for file in Bobs.signature Pic.signature
do
  diff $file ${file}.gold > /dev/null
  if [ $? -ne 0 ]
  then
    echo $file failed
    let fails=fails+1
  else
    echo $file passed
  fi
done

if [ $fails -ne 0 ]
then
  echo $fails tests failed.
  exit 1
else
  echo All tests passed.
fi
