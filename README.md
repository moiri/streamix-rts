# streamix-rts
Runtime system for the coordination language Streamix

## Installation

    make
    sudo make install

Requires
 - [`zlog`](https://github.com/HardySimpson/zlog)
 - [`pthread`](https://computing.llnl.gov/tutorials/pthreads/)
 - [`check`](https://libcheck.github.io/check/web/install.html)

## Examples
In the folder `examples` choose an example and run

    make
    ./<example>.out

Requires
 - [`smxc`](https://github.com/moiri/streamix-c) to compile the streamix code
 - [`graph2c`](https://github.com/moiri/streamix-graph2c) to translate the streamix dependency graph into c code
