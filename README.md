# MeteorShip - Setup & Build Guide 🚀

This guide explains how to install dependencies, compile, and run **MeteorShip** on Linux, Windows, and macOS.

---

## Prerequisites

> [!IMPORTANT]
> **C++26 Reflection Compiler Requirement**:
> MeteorShip uses experimental C++26 static reflection (`<meta>`, `-freflection`). The build strictly requires **GCC 16+** (`g++`). Clang and MSVC do not yet support this reflection proposal.

Before building, ensure you have:
- **GCC 16+** (`gcc` and `g++`)
- **CMake >= 4.0.2**
- **Git**
- **Python 3.14+** (with development headers/libraries)
- **vcpkg** (Microsoft C++ package manager)
- **OpenCL 1.2+ Drivers / ICD Loader** *(Optional: if no OpenCL GPU is found, the engine automatically falls back to CPU SAT collision detection)*
- **OpenGL 3.3+** compatible graphics driver

---

## Platform Setup

### 1. Linux (Native - Recommended)

#### Install System Packages

- **Ubuntu / Debian**:
  ```bash
  sudo apt update
  sudo apt install -y build-essential cmake ninja-build git curl zip unzip tar \
      pkg-config python3-dev libopencl-clhpp-headers ocl-icd-opencl-dev \
      libgl1-mesa-dev libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev
  ```

- **Arch Linux**:
  ```bash
  sudo pacman -S --needed base-devel cmake ninja git curl zip unzip tar \
      python ocl-icd opencl-headers mesa libx11 libxrandr libxcursor libxi systemd
  ```

- **Fedora**:
  ```bash
  sudo dnf install -y gcc-c++ cmake ninja-build git curl zip unzip tar \
      python3-devel ocl-icd-devel opencl-headers mesa-libGL-devel \
      libX11-devel libXrandr-devel libXcursor-devel libXi-devel systemd-devel
  ```

#### GCC 16
Verify your GCC installation:
```bash
g++ --version
# Expected: g++ (GCC) 16.x or newer
```
If your distribution repository does not package GCC 16 yet, install it via toolchain repositories (such as the Ubuntu Toolchain PPA, Arch AUR `gcc-git`, or compile GCC trunk with `--enable-languages=c,c++`).

---

### 2. Windows

Because of the GCC 16 C++26 reflection requirement, running on Windows is supported via **WSL2** (recommended) or **MSYS2**.

#### Option A: WSL2 (Recommended)
1. Install WSL2 with Ubuntu in PowerShell:
   ```powershell
   wsl --install -d Ubuntu
   ```
2. Open the WSL terminal and follow the [Linux](#1-linux-native---recommended) instructions.
3. Windows 11 with WSLg supports hardware-accelerated GUI and audio out of the box.

#### Option B: MSYS2 (Native MinGW-w64 / UCRT64)

If you prefer building a native Windows `.exe` without WSL, use **MSYS2** with the **UCRT64** environment.

##### 1. Install MSYS2
1. Download and run the installer from [msys2.org](https://www.msys2.org/).
2. Once installed, launch the **MSYS2 UCRT64** terminal (from the Start Menu or via `C:\msys64\ucrt64.exe`).

> [!WARNING]
> Always use the **MSYS2 UCRT64** shell (not *MSYS*, *MINGW32*, or *CLANG64*). The UCRT64 environment links against Microsoft's modern Universal C Runtime (`ucrtbase.dll`) and sets `/ucrt64/bin` in your `$PATH`.

##### 2. Update MSYS2 Base Packages
Run the system package manager:
```bash
pacman -Syu
```
*(If MSYS2 prompts you to close the terminal window to update core runtime packages, close the window, relaunch **MSYS2 UCRT64**, and run `pacman -Syu` once more until everything is fully up to date).*

##### 3. Install the Toolchain & Build Utilities
Install the UCRT64 GCC compiler, CMake, Ninja, Python, OpenCL headers, and archive tools required by vcpkg:
```bash
pacman -S --needed \
    base-devel \
    git \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-python \
    mingw-w64-ucrt-x86_64-pybind11 \
    mingw-w64-ucrt-x86_64-opencl-headers \
    mingw-w64-ucrt-x86_64-opencl-icd-loader \
    zip unzip tar pkg-config
```

##### 4. Verify GCC 16+ Installation
Ensure GCC 16 is active:
```bash
g++ --version
# Output should indicate GCC 16.x or newer
```
Verify that the C++26 reflection flag is recognized:
```bash
g++ -std=c++26 -freflection -dM -E - < /dev/null
```
*(If the official MSYS2 repository has not promoted GCC 16 to the main channel yet, install a GCC 16 snapshot package or compile GCC trunk with `--enable-languages=c,c++`).*

##### 5. Install and Bootstrap vcpkg for MinGW
In your **MSYS2 UCRT64** shell:
```bash
# Clone vcpkg to C:\vcpkg (accessible as /c/vcpkg in MSYS2)
git clone https://github.com/microsoft/vcpkg.git /c/vcpkg
cd /c/vcpkg
./bootstrap-vcpkg.sh

# Set the environment variable for your session
export VCPKG_ROOT=/c/vcpkg
```

##### 6. Configure and Build MeteorShip
Navigate to your cloned MeteorShip repository:
```bash
cd /c/path/to/MeteorShip
```

Run CMake with the **MinGW dynamic triplet** (`x64-mingw-dynamic`). This tells vcpkg to build dependencies with MinGW-w64 GCC instead of looking for Microsoft Visual C++ (MSVC):

```bash
cmake -B build -S . \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic \
  -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_BUILD_TYPE=Release
```

Compile the project:
```bash
cmake --build build --config Release -j$(nproc)
```

##### 7. Running the Game on Windows
Launch directly from the MSYS2 UCRT64 terminal:
```bash
cd build
./MeteorShip.exe
```

**Running Outside MSYS2 (PowerShell or Double-Clicking):**
Windows needs access to the runtime DLLs:
1. **MSYS2 GCC DLLs**: Add `C:\msys64\ucrt64\bin` to your system `PATH` (contains `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`).
2. **vcpkg Library DLLs**: Add `C:\path\to\MeteorShip\build\vcpkg_installed\x64-mingw-dynamic\bin` to your `PATH`, or copy the DLLs directly next to `MeteorShip.exe`.

---

### 3. macOS

1. Install [Homebrew](https://brew.sh/) if not already present.
2. Install build tools:
   ```bash
   brew install cmake ninja git python@3.14
   ```
3. Install GCC 16:
   ```bash
   brew install gcc
   ```
4. Point CMake to Homebrew's GCC:
   ```bash
   export CC=gcc-16
   export CXX=g++-16
   ```

---

## Step-by-Step Build and Run

### 1. Clone the Repository
```bash
git clone https://github.com/iformat/MeteorShip.git
cd MeteorShip
```

### 2. Install and Bootstrap vcpkg
If you do not already have `vcpkg` installed on your machine:
```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh # On Windows: .\vcpkg\bootstrap-vcpkg.bat
```

Set the environment variable:
```bash
export VCPKG_ROOT=~/vcpkg
```

### 3. Configure with CMake
MeteorShip uses `vcpkg.json` (manifest mode). CMake will automatically download, compile, and configure all dependencies (`SFML`, `Boost`, `sol2`, `lua`, `behaviortree-cpp`, `glad`, `imgui`, `nuklear`, `pybind11`, `opencl`):

*(Note: On Windows using MSYS2, follow the specific configuration command in [Option B](#option-b-msys2-native-mingw-w64--ucrt64) to pass the MinGW triplet).*

```bash
cmake -B build -S . \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_BUILD_TYPE=Release
```

> [!TIP]
> If your GCC 16 binary is named `g++-16`, specify `-DCMAKE_CXX_COMPILER=g++-16`.

### 4. Build the Project
```bash
cmake --build build --config Release -j$(nproc)
```

The build targets automatically copy required `assets/`, `scripts/`, and `BTreeXML/` folders into the `build/` directory.

### 5. Run the Game
```bash
cd build
./MeteorShip
```

---

## Troubleshooting & FAQ

### `This project requires GCC 16+ for C++26 reflection, found ...`
- **Cause**: CMake found a compiler other than GCC or a GCC version earlier than 16.
- **Solution**: Install GCC 16 and pass `-DCMAKE_CXX_COMPILER=g++-16` when invoking `cmake`.

### `No OpenCL GPU found; using CPU SAT collision detection`
- **Cause**: Your machine does not have an OpenCL-supported GPU or the OpenCL runtime is missing.
- **Solution**: This is a harmless notice. MeteorShip automatically activates its internal CPU SAT collision fallback.

### First build takes a long time
- **Cause**: `vcpkg` compiles dependencies (Boost, SFML, BehaviorTree.CPP, Lua, etc.) from source on initial run.
- **Solution**: Subsequent builds will reuse cached binaries in `build/vcpkg_installed` and will compile significantly faster.

### Assets / scripts failed to load at startup
- **Cause**: Launching the executable from a directory that does not contain the `assets/`, `scripts/`, or `BTreeXML/` directories.
- **Solution**: Launch the binary from the `build/` directory (where CMake copies these directories) or from the repository root.
