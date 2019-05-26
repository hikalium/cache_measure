# cache_measure
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Cache latency measuring tool for Linux.

Based on the method described in "Computer Architecture: A Quantitative Approach" by John L. Hennessy and David A. Patterson.

## Build
```
make
```

## How to use
This tool uses rdtsc instruction for measuring time.
Therefore, the cpu which is used by this program should be fixed during the measurement.

If you want to run this tool on core `$CORE`, run as follows:
```
taskset -c $CORE  ./cache_measure
```

## Author
[hikalium](https://github.com/hikalium)
