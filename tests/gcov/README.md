## Generating coverage reports

With embARC BSP, you can generate code coverage reports to analyze which parts of the code are coverded by a given testcase or application.

## Test coverage reports in embedded devices or nSIM (with GNU toolchain)
### Overview

Accompanied with the GCC compiler, GCC GCOV can assess overall test coverage of your testing program and generage coverage reports. It can help you to create more efficient, faster running code and discover untested code paths.

In embARC BSP, GCOV collects coverage profiling data in RAM (and not to a file system) while your application is running. Support for GCOV collection and reporting is limited by available RAM size and so is currently enabled for emsk and nSIM.

### Details

There are 2 settings needed to enable this feature. The first is to enable the coverage for the devices and the second is to enable the testcases. As explained earlier, the code coverage assessment is resource consuming. Please ensure your device has enough RAM before enable the coverage feature. For example with coverage assessment enabled, any simple test applications can run on a resource-tight device like IoTDK, but complex testcases could deplete all RAM resources and lead to a program crash.

To enable the device for coverage feature, select `BOARD_COVERAGE_SUPPORT`(current not use) in {board}.mk file and set `HEAPSIZE` in `makefile`.

To generate report on coverage for a particular testcase, use option `EN_COVERAGE=1`.

## Steps to generate code coverage reports

These steps will generate a HTML coverage report `gcov.html` for a testsuit.

1. Build the code with `EN_COVERAGE=1`.

```
embarc build EN_COVERAGE=1
```
2. Run the code and capture the output into a log file, generate the gcov .gcda and .gcno files from the log file that was saved , finally use gcovr to get html report.

```
python embarc_bsp/scripts/coverage.py --embarc-root embarc_bsp --app-path . --board nsim --bd-ver 10 --cur-core arcsem --toolchain gnu // (embarc build --coverage run )
```