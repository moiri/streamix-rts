# `v0.1.0`

### Bug Fixes
 - fix the tf read function (#1)

### Changes
 - Cleanup cond vars (move the cond var from fifo to channel level)

### New Features
 - Add a `unpack` function to the message callbacks
 - Allow to configure the RTS with an XML file
 - Each custom net must now define an init and cleanup function
 - Propagate termination signals in both directions
 - Use zlog categories to distinguish beteween different concept
 - Include multiple log levels
 - Use uthash to store zlog categories for nets

# `diss_final`

The initial release after completing the [dissertation](https://uhra.herts.ac.uk/handle/2299/21094).
