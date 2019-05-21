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

# `diss_final`

The initial release after completing the [dissertation](https://uhra.herts.ac.uk/handle/2299/21094).
