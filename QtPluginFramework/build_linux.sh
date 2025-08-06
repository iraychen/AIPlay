#!/bin/bash
# Build script for Qt Plugin Framework on Linux
# This script assumes Qt is installed and qmake is in the PATH

echo "Building Qt Plugin Framework..."

# Create build directories
mkdir -p build/debug
mkdir -p build/release
mkdir -p build/debug/plugins
mkdir -p build/release/plugins
mkdir -p build/debug/metadata
mkdir -p build/release/metadata
mkdir -p build/debug/config
mkdir -p build/release/config
mkdir -p build/debug/logs
mkdir -p build/release/logs

# Set build mode (debug or release)
BUILD_MODE="release"
if [ "$1" == "debug" ]; then
    BUILD_MODE="debug"
fi

echo "Building in $BUILD_MODE mode..."

# Build PluginCore first
cd PluginCore
qmake CONFIG+=$BUILD_MODE
make
if [ $? -ne 0 ]; then
    echo "Error building PluginCore"
    exit 1
fi
cd ..

# Build HostApplication
cd HostApplication
qmake CONFIG+=$BUILD_MODE
make
if [ $? -ne 0 ]; then
    echo "Error building HostApplication"
    exit 1
fi
cd ..

# Build Plugins
cd Plugins/MySqlBackup
qmake CONFIG+=$BUILD_MODE
make
if [ $? -ne 0 ]; then
    echo "Error building MySqlBackup plugin"
    exit 1
fi
cd ../..

cd Plugins/SqlServerBackup
qmake CONFIG+=$BUILD_MODE
make
if [ $? -ne 0 ]; then
    echo "Error building SqlServerBackup plugin"
    exit 1
fi
cd ../..

echo "Build completed successfully!"
echo "The application is available in build/$BUILD_MODE/HostApplication"