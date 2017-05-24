
### Making PC build with CMake

``` commandline
mkdir -p build/pc
cd build/pc
cmake ../..
make -j 8
```

### Making IMX51 build with CMake

``` commandline
mkdir -p build/imx51
cd build/imx51
cmake -DIMX51=1 ../..
make -j 8
```
