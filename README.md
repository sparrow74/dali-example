<img src="https://dalihub.github.io/images/DaliLogo320x200.png">

# Table of Contents

   * [Description](#description)
   * [Build Instructions](#build-instructions)
      * [1. Building for Ubuntu desktop](#1-building-for-ubuntu-desktop)
         * [Minimum Requirements](#minimum-requirements)
         * [Building the Repository](#building-the-repository)
      * [2. GBS Builds](#2-gbs-builds)
         * [NON-SMACK Targets](#non-smack-targets)
         * [SMACK enabled Targets](#smack-enabled-targets)
         * [DEBUG Builds](#debug-builds-1)

# Description

A simple DALi example with resources and a style.
The build configuration allows using these styles appropriately in the source code.
This repo should be modified locally to incorporate the application you wish to debug.
 * Add any header or source files to the 'src' directory.
 * Add any resources required to the 'resources/images' directory.
 * Add any styles (.json) files required to the 'resources/style' directory.

# Build Instructions

## 1. Building for Ubuntu desktop

### Requirements

 - Ubuntu 14.04 or later
 - Environment created using dali_env script in dali-core repository
 - GCC version 6

DALi requires a compiler supporting C++11 features.
Ubuntu 16.04 is the first version to offer this by default (GCC v5.4.0).

GCC version 6 is recommended since it has fixes for issues in version 5
e.g. it avoids spurious 'defined but not used' warnings in header files.

### Building the Repository

To build the repository enter the 'build/tizen' folder:

         $ cd build/tizen

Then run the following command to set up the build:

         $ cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX .

If a Debug build is required, then add -DCMAKE_BUILD_TYPE=Debug

To build run:

         $ make install -j8

## 2. GBS Builds

### NON-SMACK Targets

         $ gbs build -A [TARGET_ARCH]

### SMACK enabled Targets

         $ gbs build -A [TARGET_ARCH] --define "%enable_dali_smack_rules 1"

### DEBUG Builds

         $ gbs build -A [TARGET_ARCH] --define "%enable_debug 1"
