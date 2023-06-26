## admin-tool (experimental)

Universal tool for everscale network administration

## Build

### Clone the repository 
```bash
git clone --recurse-submodules https://github.com/tonlabs/admin-tool.git
cd admin-tool
```

### Install requirements
```
Python 3.10 or newer
GNU Make 3.81 or newer
cmake version 3.20 or newer
bash: "GNU bash, version 3.2.57(1)-release" or newer
OpenSSL (including C header files) version 1.1.1 or later
build-essential, zlib1g-dev, gperf, libreadline-dev, ccache, libmicrohttpd-dev, libcurl
g++ or clang (or another C++20 compatible compiler).
```

### Configure and compile
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j $(nproc --all) --target admin-tool
```

## License

[GNU GENERAL PUBLIC LICENSE Version 3](./LICENSE)
