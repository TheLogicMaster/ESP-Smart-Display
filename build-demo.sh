#!/bin/bash

# Build dashboard in demo mode

export VUE_APP_DEMO_MODE=true
cd ./dashboard

echo Building demo dashboard...
npm run build
rm -r ./demo
mv ./dist ./demo
cp -r ./demo-assets/* ./demo

echo Done
