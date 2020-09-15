#!/bin/bash

# Build and update server files in production mode

npm run build

echo Copying Web Interface files

find ./dist -type f ! -name "*.gz" -exec rm {} \;
cp ./dist/* ../data
