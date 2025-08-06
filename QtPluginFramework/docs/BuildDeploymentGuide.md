# Enterprise Plugin Framework - Build and Deployment Guide

This guide provides instructions for building and deploying the Enterprise Plugin Framework on Windows and Linux platforms.

## Prerequisites

### Windows

1. **Qt Framework**: Qt 5.12.0 or later (Qt 6.x is also supported)
2. **Compiler**: Microsoft Visual C++ 2017 or later
3. **Build Tools**: Qt Creator or Visual Studio with Qt VS Tools
4. **Git**: For version control (optional)

### Linux

1. **Qt Framework**: Qt 5.12.0 or later (Qt 6.x is also supported)
2. **Compiler**: GCC 7.0 or later
3. **Build Tools**: Qt Creator or qmake with make
4. **Development Packages**: 
   - `build-essential`
   - `libgl1-mesa-dev`
5. **Git**: For version control (optional)

## Directory Structure

The framework has the following directory structure:

```
QtPluginFramework/
  ├── PluginCore/              # Core plugin framework
  ├── HostApplication/         # Host application
  ├── Plugins/                 # Plugin implementations
  │   ├── MySqlBackup/         # MySQL backup plugin
  │   └── SqlServerBackup/     # SQL Server backup plugin
  ├── docs/                    # Documentation
  └── build/                   # Build output
      ├── debug/               # Debug build
      │   ├── plugins/         # Plugin libraries
      │   ├── metadata/        # Plugin metadata
      │   ├── config/          # Configuration files
      │   └── logs/            # Log files
      └── release/             # Release build
          ├── plugins/         # Plugin libraries
          ├── metadata/        # Plugin metadata
          ├── config/          # Configuration files
          └── logs/            # Log files
```

## Building with Qt Creator

### 1. Open the Project

1. Launch Qt Creator
2. Select "Open Project"
3. Navigate to the `QtPluginFramework` directory
4. Select `QtPluginFramework.pro`
5. Click "Open"

### 2. Configure the Project

1. Select the desired build configuration (Debug or Release)
2. Select the target platform (Desktop)
3. Click "Configure Project"

### 3. Build the Project

1. Click "Build" > "Build Project" or press Ctrl+B (Cmd+B on macOS)
2. Wait for the build to complete

### 4. Run the Application

1. Click "Run" or press Ctrl+R (Cmd+R on macOS)
2. The host application should start

## Building with Command Line

### Using Build Scripts

The framework includes build scripts for both Windows and Linux that handle the build process correctly.

#### Windows

```batch
cd QtPluginFramework
build_windows.bat        # For release build
build_windows.bat debug  # For debug build
```

#### Linux

```bash
cd QtPluginFramework
./build_linux.sh         # For release build
./build_linux.sh debug   # For debug build
```

### Manual Build Process

If you prefer to build manually, follow these steps to ensure the correct build order:

#### Windows

```batch
cd QtPluginFramework

REM Create build directories
mkdir build\debug
mkdir build\release
mkdir build\debug\plugins
mkdir build\release\plugins
mkdir build\debug\metadata
mkdir build\release\metadata
mkdir build\debug\config
mkdir build\release\config
mkdir build\debug\logs
mkdir build\release\logs

REM Build PluginCore first
cd PluginCore
qmake
nmake
cd ..

REM Build HostApplication
cd HostApplication
qmake
nmake
cd ..

REM Build Plugins
cd Plugins\MySqlBackup
qmake
nmake
cd ..\..

cd Plugins\SqlServerBackup
qmake
nmake
cd ..\..
```

#### Linux

```bash
cd QtPluginFramework

# Create build directories
mkdir -p build/debug build/release build/debug/plugins build/release/plugins
mkdir -p build/debug/metadata build/release/metadata
mkdir -p build/debug/config build/release/config
mkdir -p build/debug/logs build/release/logs

# Build PluginCore first
cd PluginCore
qmake
make
cd ..

# Build HostApplication
cd HostApplication
qmake
make
cd ..

# Build Plugins
cd Plugins/MySqlBackup
qmake
make
cd ../..

cd Plugins/SqlServerBackup
qmake
make
cd ../..
```

> **Important**: The PluginCore library must be built first before building the host application or plugins.

## Deployment

### Windows Deployment

1. **Build in Release Mode**: Build the project in Release mode
2. **Run windeployqt**: Use the `windeployqt` tool to gather all required Qt libraries

```batch
cd build\release
windeployqt HostApplication.exe
```

3. **Create Installation Package**: Use an installer tool like NSIS, Inno Setup, or WiX to create an installer

### Linux Deployment

1. **Build in Release Mode**: Build the project in Release mode
2. **Create AppImage or Package**: Use tools like linuxdeployqt, AppImageKit, or native packaging tools

```bash
cd build/release
linuxdeployqt HostApplication -appimage
```

## Directory Structure After Deployment

The deployed application should have the following directory structure:

```
ApplicationDirectory/
  ├── HostApplication.exe     # Main executable (Windows)
  ├── HostApplication         # Main executable (Linux)
  ├── PluginCore.dll          # Core library (Windows)
  ├── libPluginCore.so        # Core library (Linux)
  ├── plugins/                # Plugin libraries
  │   ├── MySqlBackup.dll     # MySQL backup plugin (Windows)
  │   ├── libMySqlBackup.so   # MySQL backup plugin (Linux)
  │   ├── SqlServerBackup.dll # SQL Server backup plugin (Windows)
  │   └── libSqlServerBackup.so # SQL Server backup plugin (Linux)
  ├── metadata/               # Plugin metadata
  │   ├── MySqlBackup.json    # MySQL backup plugin metadata
  │   └── SqlServerBackup.json # SQL Server backup plugin metadata
  ├── config/                 # Configuration files
  ├── logs/                   # Log files
  └── Qt libraries            # Qt DLLs or shared libraries
```

## Adding New Plugins

To add a new plugin to the deployment:

1. **Build the Plugin**: Build the plugin in the same configuration as the host application
2. **Copy Plugin Library**: Copy the plugin library to the `plugins` directory
3. **Copy Metadata**: Copy the plugin metadata file to the `metadata` directory

## Plugin Dependencies

If your plugins have external dependencies:

1. **Windows**: Include the required DLLs in the application directory
2. **Linux**: Ensure the required shared libraries are available in the system or include them with the application

## Configuration

The framework uses several configuration files:

1. **Framework Configuration**: `config/framework.json`
2. **Plugin Configuration**: `config/<pluginId>.json`

These files are created automatically when the application runs, but you can pre-configure them for deployment.

## Troubleshooting

### Missing Dependencies

If the application fails to start due to missing dependencies:

1. **Windows**: Use the Dependency Walker tool to identify missing DLLs
2. **Linux**: Use `ldd` to identify missing shared libraries

### Plugin Loading Issues

If plugins fail to load:

1. Check that the plugin libraries are in the correct location
2. Check that the metadata files are in the correct location
3. Check the log files for error messages
4. Verify that all plugin dependencies are satisfied

## Updating the Application

To update an existing installation:

1. **Windows**: Replace the executable, libraries, and plugins with the new versions
2. **Linux**: Replace the executable, libraries, and plugins with the new versions

Configuration files and logs should be preserved during updates.

## Conclusion

This guide provides the basics for building and deploying the Enterprise Plugin Framework. For more detailed information, refer to the Qt documentation on deployment and the specific requirements of your target platforms.