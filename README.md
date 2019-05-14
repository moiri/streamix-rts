# streamix-rts
Runtime system for the coordination language Streamix

## Installation

    make
    sudo make install

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
In the folder `examples` choose an example and run

    make
    ./<example>.out

Requires
 - [`smxc`](https://github.com/moiri/streamix-c) to compile the streamix code
 - [`graph2c`](https://github.com/moiri/streamix-graph2c) to translate the streamix dependency graph into c code
