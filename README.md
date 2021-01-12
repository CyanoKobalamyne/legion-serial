# serial-legion

This is a (partial) implementation of the [Legion API](https://legion.stanford.edu/doxygen/) for testing and benchmarking. It does not do any kind of synchronization, so it incurs minimal overhead, but it will also yield incorrect results if multiple tasks modify the same data concurrently.
