# Streamix RTS `smxrts`

The runtime system (RTS) for the coordination language Streamix.

## Installation

A Debian package is distributed with the repository. Use the command

    sudo apt install __path_to_deb__

where `__path_to_deb__` defines the path to the local Debian package.

## Building

To build the RTS from scratch run the following commands:

    git clone --recursive https://github.com/moiri/streamix-rts.git git-smxrts
    make
    sudo make install

Note that time-triggered nets are executed with RT-Tasks. In order to prevent
priority-inversion the logging is locked by a mutex (instead of a simple
rw-lock). This is not necessary if the Streamix network does not have any tt
nets. In this case the mutex locking of the RTS can be disabled at compile time
with `make unsafe`.

Requires
 - [`zlog`](https://github.com/HardySimpson/zlog)
    is added as a git submodule and can be used to compile from source.

    ```
    cd zlog
    make
    sudo make install
    ```

    **Important Note:** Do not statically link `libzlog`, otherwise the
    resulting software must be published under
    [LGPL-v2.1](https://choosealicense.com/licenses/lgpl-2.1/).

    **Impotant Note:** In the context of a TPF installation do **not** use this
    submodule but refer to the zlog dependency package on the internal
    [GitLab](http://phhum-a209-cp.unibe.ch:10012/SMX/SMX-deps/SMX-zlog).

 - [`libbson`](http://mongoc.org/libbson/current/index.html)

    ```
    sudo apt install libbson-dev
    ```

 - [`pthread`](https://computing.llnl.gov/tutorials/pthreads/)

    ```
    sudo apt install libpthread-stubs0-dev
    ```

    **Important Note:** Do not statically link `libpthread`, otherwise the
    resulting software must be published under
    [LGPL-v2.1](https://choosealicense.com/licenses/lgpl-2.1/).

 - [`lttng`](https://lttng.org/)

    ```
    sudo apt-get install lttng-tools
    sudo apt-get install lttng-modules-dkms
    sudo apt-get install liblttng-ust-dev
    ```

## Examples
Some example can be found in the [root repository of Streamix](https://github.com/moiri/streamix).
Refer to this repo for compilation instructions.

## Troubleshooting
zlog fails if it does not have read access to the configuration file. Make sure
that the default configuration file as well as a customized version is writable
by the user executing a Streamix application.
