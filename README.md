# streamix-rts
Runtime system for the coordination language Streamix

Requires
 - [`zlog`](https://github.com/HardySimpson/zlog)
 - [`pthread`](https://computing.llnl.gov/tutorials/pthreads/)

## Installation

    make
    sudo make install


## Examples
In the folder `examples` choose an <example> and run

    make
    ./<example>.out

Requires
 - [`smxc`](https://github.com/moiri/streamix-c) to compile the streamix code
 - [`gml2c`](https://github.com/moiri/streamix-gml2c) to translate the gml dependency graph into c code
