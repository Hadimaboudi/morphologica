# morphologica

Library code used in models developed by Stuart P. Wilson and co-workers

This code builds a shared library called libmorphologica.

It installs the library on your system, along with the required header
files.

Code is (or shortly will be) enclosed in the "morph" namespace.

It requires OpenCV, Armadillo and OpenGL headers to compile, and
programs linked with libmorphologica will also need to link to those
dependencies. You will also need the cmake program and a C++ compiler.

To install these dependencies on Ubuntu or Debian Linux, you can do:

```sh
sudo apt install build-essential cmake libopencv-dev libarmadillo-dev libglu1-mesa-dev
```

To build, it's the usual CMake process:

```sh
cd morphologica
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4
ctest
sudo make install
sudo ldconfig # Probably Linux specific! Mac alternative?
```

Note the call to ldconfig at the end there, which makes sure that
libmorphologica is available to your system's dynamic linker. On
Linux, that means running ldconfig (assuming that the
CMAKE\_INSTALL\_PREFIX of /usr/local is already in your dynamic
linker's search path) as in the example above. If you installed
elsewhere, then you probably know how to set LD\_CONFIG\_PATH
correctly (or at least you can now search up how to do that).
