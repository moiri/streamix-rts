# streamix-rts
Runtime system for the coordination language Streamix

## Installation

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
 - [`libxml2`](http://www.xmlsoft.org/)

    ```
    sudo apt update
    sudo apt install libxml2-dev
    ```

 - [`uthash`](https://github.com/troydhanson/uthash)

    ```
    sudo apt update
    sudo apt install uthash-dev
    ```

 - [`pthread`](https://computing.llnl.gov/tutorials/pthreads/)

    ```
    sudo apt update
    sudo apt install libpthread-stubs0-dev
    ```

## Examples
Some example can be found in the [root repository of Streamix](https://github.com/moiri/streamix).
Refer to this repo for compilation instructions.
