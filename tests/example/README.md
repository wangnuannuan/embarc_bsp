## Running unittest located under embarc_bsp

### Structure

- `middleware/common/embARC_test.c` - a lightweighted unit test framework based on CTest and Contiki App\unitest
- `tests/` - folder of testcases that are based on `middleware/common/embARC_test.c` and gcov
- `scripts/testsuit.py` - a python script that scans testcases and generates test project
- `scripts/tcoverage.py` - a python script that debugs test project with fdb and generates code coverage report

### Environment Preparation
#### Prerequisites
- `embarc_cli`
- `gcovr` - a python module that summarizes the coverage in simple reports

### Running the unittest

Now you can generate a test project on your terminal with the following command:

`python <bsp_root/scripts/testsuit.py> --embarc-root <bsp_root> --test-case tests/arc --test-suit arc_timer`

This command scans files under `<bsp_root>/tests/arc` to find testcase and generates test project including `main.c` and `makefile`. Here are the default build configures:
```
BOARD=nsim
BD_VER=10
CUR_CORE=arcem
TOOLCHAIN=gnu
```

Finally, run and generate code coverage report with the following command:

`python <bsp_root/scripts/coverage.py> --embarc-root <bsp_root> --app-path <test_root> --board nsim --bd-ver 10 --cur-core arcsem --toolchain gnu`

The log is shown below:

```
$ python ../../scripts/coverage.py --embarc-root ../../ --app-path . --board nsim --bd-ver 10 --cur-core arcsem --toolchain gnu
[embARC] Start to run ...
"Download & Run obj_nsim_10/gnu_arcsem/unit_test_gnu_arcsem.elf"
nsimdrv -p nsim_emt=1 -propsfile obj_nsim_10/gnu_arcsem/embARC_generated/nsim.props obj_nsim_10/gnu_arcsem/unit_test_gnu_arcsem.elf
-----------------------------------------------------------
 ____                                _ ____
|  _ \ _____      _____ _ __ ___  __| | __ ) _   _
| |_) / _ \ \ /\ / / _ \ '__/ _ \/ _` |  _ \| | | |
|  __/ (_) \ V  V /  __/ | |  __/ (_| | |_) | |_| |
|_|   \___/ \_/\_/ \___|_|  \___|\__,_|____/ \__, |
                                             |___/
                     _       _    ____   ____
       ___ _ __ ___ | |__   / \  |  _ \ / ___|
      / _ \ '_ ` _ \| '_ \ / _ \ | |_) | |
     |  __/ | | | | | |_) / ___ \|  _ <| |___
      \___|_| |_| |_|_.__/_/   \_\_| \_\\____|
------------------------------------------------------------

embARC Build Time: Oct 23 2019, 15:15:41
Compiler Version: ARC GNU, 8.3.1 20190225
----embARC unit tests----
start:0x800023b0, end:0x800025d0, total:8
TEST 1/8 arc_timer:timer_present
 [OK]
TEST 2/8 arc_timer:timer_start
 [OK]
TEST 3/8 arc_timer:timer_stop
 [OK]
TEST 4/8 arc_timer:timer_current
 [OK]
TEST 5/8 arc_timer:timer_int_clear
 [OK]
TEST 6/8 arc_timer:secure_timer_start
 [OK]
TEST 7/8 arc_timer:secure_timer_current
 [OK]
TEST 8/8 arc_timer:timer_calibrate_delay
 [OK]
RESULTS: 8 tests (8 ok, 0 failed, 0 skipped)
----embARC unit test end----
[embARC] Dump coverage data to file ...
[embARC] Generating gcda files
```