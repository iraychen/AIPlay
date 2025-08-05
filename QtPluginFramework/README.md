# Enterprise Plugin Framework

A comprehensive Qt C++ plugin framework for enterprise applications, featuring high extensibility, stability, and security.

## Overview

The Enterprise Plugin Framework provides a robust foundation for building modular, plugin-based applications. It includes a complete plugin management system, host application, and example plugins.

## Features

### Plugin Core System

- Unified plugin interface with JSON metadata management
- Plugin lifecycle management (load, initialize, activate, deactivate, shutdown, unload)
- Plugin dependency management with dependency detection and load order control
- Version compatibility checking

### Core Functionality

- Plugin Manager with dynamic loading and hot-swapping capabilities
- Secure inter-plugin communication mechanism
- Centralized logging system
- Configuration management for framework and individual plugins
- Permission control system
- Unified exception handling

### Host Application

- Main window with dynamic plugin integration
- Menu, toolbar, and status bar integration for plugins
- Plugin management interface
- Plugin event and status monitoring

### Example Plugins

- MySQL Database Backup Plugin
- SQL Server Database Backup Plugin

## Directory Structure

```
QtPluginFramework/
  ├── PluginCore/              # Core plugin framework
  ├── HostApplication/         # Host application
  ├── Plugins/                 # Plugin implementations
  │   ├── MySqlBackup/         # MySQL backup plugin
  │   └── SqlServerBackup/     # SQL Server backup plugin
  ├── docs/                    # Documentation
  └── build/                   # Build output
```

## Documentation

- [Framework Design Document](docs/FrameworkDesign.md): Detailed explanation of the framework architecture and design decisions
- [Plugin Development Guide](docs/PluginDevelopmentGuide.md): Guide for developing new plugins
- [Build and Deployment Guide](docs/BuildDeploymentGuide.md): Instructions for building and deploying the framework

## Requirements

- Qt 5.12.0 or later (Qt 6.x is also supported)
- C++11 compatible compiler
- Windows, Linux, or macOS

## Building

### Using Qt Creator

1. Open `QtPluginFramework.pro` in Qt Creator
2. Configure the project for your target platform
3. Build the project

### Using Command Line

```bash
cd QtPluginFramework
mkdir build
cd build
qmake ../QtPluginFramework.pro
make  # or nmake on Windows
```

## Usage

After building, run the host application:

```bash
cd build/debug  # or build/release
./HostApplication  # or HostApplication.exe on Windows
```

Use the "Plugins" menu or the plugin manager dialog to load, activate, and manage plugins.

## Developing Plugins

To create a new plugin:

1. Create a new directory in the `Plugins` directory
2. Implement the `IPlugin` interface
3. Create a JSON metadata file
4. Add the plugin to the `Plugins.pro` file

See the [Plugin Development Guide](docs/PluginDevelopmentGuide.md) for detailed instructions.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Qt Framework for providing the foundation
- Enterprise application development best practices