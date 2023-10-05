# `v0.10.2` (latest)

### Bug Fixes

 - Fix segfault on app name if a map is passed but no `experiment_id` is set.


-------------------
# `v0.10.1`

### Bug Fixes

 - Fix macro signature for setting a channel read timeout in test modus.


-------------------
# `v0.10.0`

### New Features

 - Add logging to mapping functions.
 - Allow to set a source map prefix to distinguish between different source
   payloads.
 - Allow to disable a net through the configuration option `is_disabled`.

### Changes

 - A new major version is required due to potential alignment problems in
   structures.

### Bug Fixes

 - Fix memory leaks when RTS initialisation fails.


-------------------
# `v0.9.3` (deprecated)

### New Features

 - Allow to disable a net through the configuration option `is_disabled`.


-------------------
# `v0.9.2`

### New Features

 - Allow to set the backup buffer of a channel through an input port.


-------------------
# `v0.9.1`

### Bug Fixes

 - Fix log dependency


-------------------
# `v0.9.0`

### Changes

 - Versioning dvl package
 - Make static config pre-requisite for pre-initialisation

### New Features

 - Allow to pass a read-only JSON structure to the RTS.
 - Extends data mappings with a key to later identify a mapping.


-------------------
# `v0.8.4`

### Bug Fixes

 - Fix channel timeout handling (nsec values were simply added without taking
   spilling to seconds into account)
 - Fix memory leak in config maps.


-------------------
# `v0.8.3`

### Changes

 - Only log FIFO tail overwrite to `notice` if the FIFO length `>1`,
   use `info` otherwise.


-------------------
# `v0.8.2`

### Bug Fixes

 - Fix a problem with data mappings when payloads or source paths are not
   defined.


-------------------
# `v0.8.1`

### New Features

 - Improve config data maps to increase usability


-------------------
# `v0.8.0`

### Changes

 - `smx_program_init()` now expects additional parameters `app_conf_maps` and
   `app_conf_map_count`.
 - install include files to versioned folder.
 - remove debian stuff to handle this in a seperate wrapper.

### New Features

 - Add init config getter functions for a more convenient way to detect missing
   values.
 - Add strerror function for config errors.
 - Add strerror function for channel errors.
 - Allow config maps to overwrite config settings. This is useful to create
   several configuration flavors without changing the default config.

### Bug Fixes

 - Improve config read function for double values.


-------------------
# `v0.7.2`

### New Features

 - Add getter defines for net attributes (loop count, thread prio).


-------------------
# `v0.7.1`

### Bug Fixes

 - Terminate a routing node if all input channels have terminated and not
   whenever any input channel terminates.


-------------------
# `v0.7.0`

### Bug Fixes

 - fix concurrency issue with collector mutex: move collector operations inside
   channel mutex locks. This will avoid unnecessary executions of `smx_rn` nets.

### Changes

 - Add more lttng probes to allow for tracing blocking operations.
 - Disable mutex locks on log by default (dangerous in when using TT nets).

### New Features

 - Add net config option `expected_rate` which allows to log warning if the
   net rate is lower or higher than expected.
 - Allow to allocate a state which can be shared between nets.


-------------------
# `v0.6.2`

### Changes

 - Log a write warning if a fifo buffer was filled (and length is bigger than
   1).


-------------------
# `v0.6.1`

### Changes

 - Pass net pointer to the content filter.
 - Only log failed write channel access to terminated consumer once.


-------------------
# `v0.6.0`

### Bug Fixes

 - Fix the counting mechanism in the test functions to allow for multiple reads
   or writes in one net execution iteration.
 - Provide a test data reader which is memory save. The old one is kept for
   legacy reasons.

### Changes

 - Accumulate overwrite fifo messages to avoid log bursts.
 - Change type of message ID from `unsigned long` to `unsigned long long`.
 - In order to catch all test read and write operations the test file in the box
   implementation must now overwrite the RTS versions. The redefinitions in the
   RTS have been removed.


-------------------
# `v0.5.6`

### Bug Fixes

 - fix open port connection problem when open port is omitted in Streamix
 - fix zlog initialisation check

### Changes

 - reduce log level for rate-controller messages from notice to info
 - add base message type definitions


-------------------
# `v0.5.5`

### Bug Fixes

 - fix test channel write macro


-------------------
# `v0.5.4`

### Bug Fixes

 - fix test dataset indexing

### Changes

 - log net execution count


-------------------
# `v0.5.3`

### Changes

 - allocate/free channel name


-------------------
# `v0.5.2`

### Changes

 - allocate/free net and impl name


-------------------
# `v0.5.1`

### Bug Fixes

 - fix memory leak
 - protect content filter setter with mutex


-------------------
# `v0.5.0`

### Bug Fixes

 - improve log messages
 - fix config parser

### Changes

 - handle the log config file in a similar way as the app config file (by
   passing it as app parameter)

### New Features

 - add a log level 'event'
 - add support for channel read and write timeouts
 - allow to dynamically configure each net


-------------------
# `v0.4.0`

### Bug Fixes

 - check whether channel is null before setting type filter
 - use the maximum net degree from the signature and the graph

### Changes

 - don't use pthread functions in header to avoid unnecessary dependencies
 - add log messages before and after init barrier
 - use specific zlog version
 - add debian revision to config
 - remove default zlog file (added to zlog package)
 - fix doxygen config
 - use debian install locations
 - seperate config.mk and Makefile
 - update to new debian files


-------------------
# `v0.3.2`

### Bug Fixes

 - now, general boolean properties of a net are read correctly.
 - first check message type filter, then check message content filter.


-------------------
# `v0.3.1`

### Changes

 - Log version numbers on level NOTICE.
 - Log RTS version number.


-------------------
# `v0.3.0`

### Bug Fixes

 - fix some typos
 - exclude channel macro to set content filter from testing clause

### New Features

 - Allow to configure whether a message is backed up in a decoupled channel.


-------------------
# `v0.2.1`

### Bug Fixes

 - fix deadline miss reports on non-TT nets (#16)

### Changes

 - Remove XML configuration
 - Remove Streamix-based profiling
 - Improve deadline detection handling (#16)
 - Remove the message type `tsmem` from the RTS and created an external library
   instead (`libsmxmsgtsmem`)

### New Features

 - Allow to configure the RTS with a JSON file instead of XML (#17)
 - Create a small configuration API which allows to easily access keys in a
   JSON configuration file
 - Allow to address each net instance in configuration (#14)
 - Allow for sub-configuration files (#7)
 - Add profiling capabilities with [lttng](https://lttng.org/) (#13)
 - Allow to check for the reason if a channel read or write operation fails (#8)
 - Add a test environment that allows to run a single box implementation (#12)
 - Add a type field to the message structure.
 - Add a macro to access the implementation name.
 - Add a utility function to filter message types.
 - Add generic message type and message content filter functionality that acts
   upon writing a message to a channel. For reasons of performance, the type
   filter can be disabled globally with a boolean config option `type_filter`.


-------------------
# `v0.1.0`

### Bug Fixes

 - fix the TF read function (#1)
 - fix the termination behaviour of TF
 - fix the termination behaviour of RN
 - Handle potential seg-faults by checking for null pointers consistently
 - fix mutex locking and unlocking
 - fix collector counter

### Changes

 - Cleanup cond vars (move the cond var from fifo to channel level)
 - Use a common net structure to store information every net has
 - Create channel ends to better distinguish between read and write operations
 - Add new FIFO types to handle special case of TFs (see #1)
 - Cope with open ports by allowing a channel to be `NULL`
 - Pass net handler to msg macros and cleanup function
 - Restructuring of `smx_tf` to make it more similar to a normal net
 - Synchronize initialisation with a `pthread_barrier`
 - Adapt to cope with c template files generated by `smxrtsp`
 - Change the license from GPL-v3.0 to MPL-v2.0

### New Features

 - Add a `unpack` function to the message callbacks
 - Allow to configure the RTS with an XML file (path to the file can be passed
   by parameter)
 - Pass net-specific configuration to the net instance
 - Each custom net must now define an init and cleanup function
 - Propagate termination signals in both directions
 - Use zlog categories to distinguish between different concept
 - Include multiple log levels
 - Make the routing node fair when reading from inputs (a rn has now state)
 - Allow to configure whether a TF copies msgs on DL miss of the producer or
   sends a NULL msg (if copy is enabled every msg has to be backed up)
 - Run TF and TF-framed nets as RT-Tasks with different priorities
 - Add profiling capabilities (requires a box to capture the profiling events,
   e.g. `smx_mogo`)
 - At the end of an app execution log execution time and box execution counts


-------------------
# `diss_final`

The initial release after completing the [dissertation](https://uhra.herts.ac.uk/handle/2299/21094).
