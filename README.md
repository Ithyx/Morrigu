
# Morrigu
![Morrigu_logo_temp](src/Morrigu/resources/Logo_Banner_TEMP.jpg)

# How to install: 
![MRG_Ubuntu](https://github.com/Ithyx/Morrigu/workflows/MRG_Ubuntu/badge.svg)
![MRG_Windows](https://github.com/Ithyx/Morrigu/workflows/MRG_Windows/badge.svg)
![MRG_MacOS](https://github.com/Ithyx/Morrigu/workflows/MRG_MacOS/badge.svg)

Morrigu uses CMake as build tool and Conan as packet manager. This makes the setup process relatively easy.  
You need [CMake 3.15+](https://cmake.org/download/), and a somewhat recent version of [Conan](https://conan.io/downloads.html) (tested on 1.26.1) with the follwing remotes:
 * The [bincrafters remote](https://docs.conan.io/en/latest/uploading_packages/remotes.html#bincrafters);
 * A [personal remote](https://bintray.com/ithyx/imgui) that contains a ImGui docking recipe.

To install a remote, follow the instructions that appear when you click "SET ME UP!" on bintray (or the conan doc to install the bincrafters remote). You do not have to login to anything to use these remotes.

Morrigu uses the `cmake_multi` conan generator, so you should be able to install both Debug and Release versions of the dependencies and use both without having to switch context. To make an "out of source build", the steps are then simply:
```bash
mkdir build
cd build
conan install .. -s build_type=Debug --build=missing
conan install .. -s build_type=Release --build=missing
cmake -G "<GENERATOR>" .. [-DCMAKE_BUILD_TYPE={Debug|Release}]
```
Where `<GENERATOR>` is a [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) supported by the version you are using. Not providing a `G` flag should fall back to a default value (defined by your cmake installation). IMPORTANT: Unless the generator you are trying to build for does support multi-configuration environment (Visual Studio, Xcode), you will have to tell cmake which version you would like to build, and you will HAVE to put in the part in square brackets in the command above.

To actually build, you can use the tool that cmake generated for, or use the universal build interface provided by cmake :
```bash
cmake --build . [--config <CONFIG>]
```
If your generator of choice supports multi-configuration environments , you need to specify the `<CONFIG>` param. It is either `Debug` or `Release`. You don't need to specify anything here otherwise.

Note: The `conan install` lines of the script might not need the `--build=missing` option to build necessary packages from source as debug or release if you already have installed that version before. It is also important to note that you HAVE to install both versions of the dependencies to be able to use CMake.

If you are building spdlog and/or fmt make sure to setup your conan for a GCC version newer than 5.0. From [conan's getting started](https://docs.conan.io/en/latest/getting_started.html):
```bash
conan profile new default --detect  # Generates default profile detecting GCC and sets old ABI
conan profile update settings.compiler.libcxx=libstdc++11 default  # Sets libcxx to C++11 ABI
```

Another important note: This project uses the `filesystem` standard library. While this is part of C++17, GCC-6 and more importantly GCC-7, which are considered C++17 compliant, do not recognize the `<filesystem>` header (instead it's `<experimental/filesystem>` + a special compile flag). Thus, if you want to modify the source code, go ahead, but I will only support GCC-8 and above.
