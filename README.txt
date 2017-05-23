
Compiling for host:
   $ NOCONFIGURE=1 ./autogen.sh    [skip if building from tarball]
   $ ./configure --prefix=/usr
   $ make
   $ make install

Cross compiling for iMX51:
   $ NOCONFIGURE=1 ./autogen.sh    [skip if building from tarball]
   $ ./configure                                                            \
         --host=armv7l-ims-linux-gnueabi                                    \
         --prefix=/opt/timesys/i_MX51/toolchain/                            \
         PKG_CONFIG_PATH=/opt/timesys/i_MX51/toolchain/lib/pkgconfig        \
         CC=/opt/timesys/i_MX51/toolchain/bin/armv7l-ims-linux-gnueabi-gcc  \
         CXX=/opt/timesys/i_MX51/toolchain/bin/armv7l-ims-linux-gnueabi-g++ \
         CXXFLAGS="-DIMX51"
   $ make
   $ make install

Cross compiling for iMX6:
   $ NOCONFIGURE=1 ./autogen.sh    [skip if building from tarball]
   $ ./configure                                                           \
         --host=armv7l-ims-linux-gnueabi                                   \
         --prefix=/opt/timesys/i_MX6/toolchain/                            \
         PKG_CONFIG_PATH=/opt/timesys/i_MX6/toolchain/lib/pkgconfig        \
         CC=/opt/timesys/i_MX6/toolchain/bin/armv7l-ims-linux-gnueabi-gcc  \
         CXX=/opt/timesys/i_MX6/toolchain/bin/armv7l-ims-linux-gnueabi-g++ \
         CXXFLAGS="-DIMX6"
   $ make
   $ make install
