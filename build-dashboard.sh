#!/bin/bash

# Build and update server files in production mode

cd ./dashboard

echo Building dashboard...
npm run build

echo Copying dashboard files...
find ./dist -type f \( ! -iname \*.gz \) -exec gzip -f {} \;
find ./dist -type f \( ! -iname \*.gz \) -exec rm {} \;
find ./dist -type f \( -iname \*.map.gz \) -exec rm {} \;
echo $(grep DASHBOARD_VERSION .env | cut -d '=' -f2) >> ./dist/version.txt
rm -r ../display/data
mkdir ../display/data
cp -r ./dist/* ../display/data
cp -r ../fs-data/* ../display/data
echo Done
