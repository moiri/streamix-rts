# `v0.1.0`

### Bug Fixes
 - fix the TF read function (#1)
 - fix the termination behaviour of TF
 - fix the termination behaviour of RN
 - Handle potential seg-faults by checking for null pointers consistently

### Changes
 - Cleanup cond vars (move the cond var from fifo to channel level)
 - Use a common net structure to store information every net has
 - Create channel ends to better distinguish between read and write operations
 - Add new FIFO types to handle special case of TFs (see #1)

### New Features
 - Add a `unpack` function to the message callbacks
 - Allow to configure the RTS with an XML file
 - Pass net-specific configuration to the net instance
 - Each custom net must now define an init and cleanup function
 - Propagate termination signals in both directions
 - Use zlog categories to distinguish between different concept
 - Include multiple log levels
 - Make the routing node fair when reading from inputs (a rn has now state)
 - Allow to configure whether a TF copies msgs on DL miss of the producer or
   sends a NULL msg (if copy is enabled every msg has to be backed up)
 - Run TF and TF-framed nets as RT-Tasks with different priorities
 - Add profiling capabilities (requires a `smx_mongo` box in the streamix nw)

# `diss_final`

The initial release after completing the [dissertation](https://uhra.herts.ac.uk/handle/2299/21094).
