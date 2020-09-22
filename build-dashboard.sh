#!/bin/bash

# Build and update server files in production mode

cd ./led-matrix-dashboard
echo Checking dependencies...
npm install

echo Building dashboard...
npm run build

echo Copying dashboard files...
find ./dist -type f \( -iname \*.jpg -o -iname \*.jpeg -o -iname \*.png \) -exec gzip {} \;
find ./dist -type f \( ! -iname \*.gz \) -exec rm {} \;
find ./dist -type f \( -iname \*.map.gz \) -exec rm {} \;
cp -r ./dist/* ../data
echo Done
