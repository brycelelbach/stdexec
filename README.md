# Senders - A Standard Model for Asynchronous Execution in C++

`stdexec` is an experimental reference implementation of the _Senders_ model of asynchronous programming proposed by [**P2300 - `std::execution`**](http://wg21.link/p2300) for adoption into the C++ Standard.

**Purpose of this Repository:**
1. Provide a proof-of-concept implementation of the design proposed in [P2300](http://wg21.link/p2300).
2. Provide early access to developers looking to experiment with the Sender model.
3. Colloborate with those interested in participating or contributing to the design of P2300 (contributions welcome!).

## Disclaimer

`stdexec` is experimental in nature and subject to change without warning.
The authors and NVIDIA do not guarantee that this code is fit for any purpose whatsoever.

[![CI](https://github.com/NVIDIA/stdexec/workflows/CI/badge.svg)](https://github.com/NVIDIA/stdexec/actions)

## Example

Below is a simple program that executes three senders concurrently on a thread pool.
Try it live on [godbolt!](https://godbolt.org/z/3cseorf7M).

```c++
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

int main()
{
    // Declare a pool of 3 worker threads:
    exec::static_thread_pool pool(3);

    // Get a handle to the thread pool:
    auto sched = pool.get_scheduler();

    // Describe some work:
    // Creates 3 sender pipelines that are executed concurrently by passing to `when_all`
    // Each sender is scheduled on `sched` using `on` and starts with `just(n)` that creates a
    // Sender that just forwards `n` to the next sender.
    // After `just(n)`, we chain `then(fun)` which invokes `fun` using the value provided from `just()`
    // Note: No work actually happens here. Everything is lazy and `work` is just an object that statically
    // represents the work to later be executed
    auto fun = [](int i) { return i*i; };
    auto work = stdexec::when_all(
        stdexec::on(sched, stdexec::just(0) | stdexec::then(fun)),
        stdexec::on(sched, stdexec::just(1) | stdexec::then(fun)),
        stdexec::on(sched, stdexec::just(2) | stdexec::then(fun))
    );

    // Launch the work and wait for the result
    auto [i, j, k] = stdexec::sync_wait(std::move(work)).value();

    // Print the results:
    std::printf("%d %d %d\n", i, j, k);
}
```

## Resources
- [HPCWire Article](https://www.hpcwire.com/2022/12/05/new-c-sender-library-enables-portable-asynchrony/): Provides a high-level overview of the Sender model and its benefits
- [NVIDIA HPC SDK Documentation](https://docs.nvidia.com/hpc-sdk/index.html): Documentation for the NVIDIA HPC SDK
- [P2300 - `std::execution`](https://wg21.link/p2300): Senders proposal to C++ Standard

## Structure

This library is header-only, so all the source code can be found in the `include/` directory. The physical and logical structure of the code can be summarized by the following table:

| Kind | Path | Namespace |
|------|------|-----------|
| Things approved for the C++ standard | `<stdexec/...>` | `::stdexec` |
| Generic additions and extensions | `<exec/...>` | `::exec` |
| NVIDIA-specific extensions and customizations | <code>&lt;nvexec/...&gt;</code> | <code>::nvexec</code> |
| | |

## How to get `stdexec`

There are a few ways to get `stdexec`:
1. Clone from GitHub
   - `git clone https://github.com/NVIDIA/stdexec.git`
2. Download the [NVIDIA HPC SDK starting with 22.11](https://developer.nvidia.com/nvidia-hpc-sdk-releases)
3. (Recommended) Use [CMake Package Manager (CPM)](https://github.com/cpm-cmake/CPM.cmake) to automatically pull `stdexec` as part of your CMake project. [See below](#cmake-package-manager-cpm) for more information.

You can also try it directly on [godbolt.org](https://godbolt.org/z/acaE93xq3) where it is available as a C++ library or via the nvc++ compiler starting with version 22.11 ([see below](#nvhpc-sdk) for more details).

## Using `stdexec`

### Requirements

`stdexec` requires compiling with C++20 (`-std=c++20`) but otherwise does not have any dependencies and only requires a sufficiently new compiler:
- clang 12+
- gcc 11+
- [nvc++ 22.11+](https://developer.nvidia.com/nvidia-hpc-sdk-releases) (required for GPU support)

How you configure your environment to use `stdexec` depends on how you got `stdexec`.

### NVHPC SDK

Starting with the 22.11 release of the [NVHPC SDK](https://developer.nvidia.com/nvidia-hpc-sdk-releases), `stdexec` is available as an experimental, opt-in feature. Using `stdexec` requires specifying the `--experimental-stdpar` flag to `nvc++` to make the `stdexec` headers available on the include path. You can then include any `stdexec` header as normal: `#include <stdexec/...>`, `#include <nvexec/...>`.  See [godbolt example](https://godbolt.org/z/qc1h3sqEv).

GPU features additionally require specifying `-stdpar=gpu`. For more details, see [GPU Support](#gpu-support).

### GitHub

As a header-only C++ library, technically all one needs to do is add the `stdexec` `include/` directory to your include path as `-I<stdexec root>/include` in addition to specifying any necessary compile options.

For simplicity, we reocmmend using the [CMake targets](#cmake) that `stdexec` provides as they encapsulate the necessary configuration.

#### cmake

If your project uses CMake, then after cloning `stdexec` simply add the following to your `CMakeLists.txt`:
```
add_subdirectory(<stdexec root>)
```

This will make the `stdexec::stdexec` target available for linking to your project as:

```
target_link_libraries(my_project PRIVATE stdexec::stdexec)
```

This target encapsulates all of the necessary configuration and compiler flags for using `stdexec`.


#### CMake Package Manager (CPM)

To further simplify obtaining and including `stdexec` in your CMake project, we recommend using [CMake Package Manager (CPM)](https://github.com/cpm-cmake/CPM.cmake) to fetch and configure `stdexec`.

Complete example:
```
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(stdexecExample)

# Get CPM
# For more information on how to add CPM to your project, see: https://github.com/cpm-cmake/CPM.cmake#adding-cpm
include(CPM.cmake)

CPMAddPackage(
  NAME stdexec
  GITHUB_REPOSITORY NVIDIA/stdexec
  GIT_TAG main # This will always pull the latest code from the `main` branch. You may also use a specific release version or tag
)

add_executable(main example.cpp)

target_link_libraries(main stdexec::stdexec)
```

### GPU Support

TODO


## Building

`stdexec` is a header-only library and does not require building anything.

This section is only relevant if you wish to build the `stdexec` tests or examples.

The following tools are needed:

* [`CMake`](https://cmake.org/)
* One of the following supported C++ compilers:
  * GCC 11+
  * clang 12+
  * nvc++ 22.11

Perform the following actions:

```bash
# Configure the project
cmake -S . -B build -G<gen>
# Build the project
cmake --build build
```

Here, `<gen>` can be `Ninja`, `"Unix Makefiles"`, `XCode`, `"Visual Studio 15 Win64"`, etc.

### Specifying the compiler

You can set the C++ compiler via `-D CMAKE_CXX_COMPILER`:

```bash
# Use GCC:
cmake -S . -B build/g++ -DCMAKE_CXX_COMPILER=$(which g++)
cmake --build build/g++

# Or clang:
cmake -S . -B build/clang++ -DCMAKE_CXX_COMPILER=$(which clang++)
cmake --build build/clang++
```

### Specifying the stdlib

If you want to use `libc++` with clang instead of `libstdc++`, you can specify the standard library as follows:

```bash
# Do the actual build
cmake -S . -B build/clang++ -G<gen> \
    -DCMAKE_CXX_FLAGS=-stdlib=libc++ \
    -DCMAKE_CXX_COMPILER=$(which clang++)

cmake --build build/clang++
```
