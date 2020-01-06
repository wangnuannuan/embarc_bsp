TEST_SUITE ?=
## embARC application makefile template ##
### You can copy this file to your application folder
### and rename it to makefile.
##

# Application name
APPL ?= unit_test

# Optimization Level
# Please Refer to toolchain_xxx.mk for this option
OLEVEL ?= O3

##
# Current Board And Core
##
BOARD ?= nsim
BD_VER ?= 10
CUR_CORE ?= arcem
EN_COVERAGE=1
##
# select debugging jtag
##
JTAG ?= usb

##
# Set toolchain
##
TOOLCHAIN ?= gnu

##
# Uncomment following options
# if you want to set your own heap and stack size
# Default settings see options.mk
##
HEAPSZ ?= 30720 # 10240
#STACKSZ ?= 8192

LIB_SEL ?= clib console unittest
#
# root dir of embARC
#
EMBARC_ROOT = ../..


# application source dirs
APPL_CSRC_DIR = .
APPL_ASMSRC_DIR = .

# application include dirs
APPL_INC_DIR = .

# application defines
APPL_DEFINES = -DEMBARC_TEST_SUITE=\"$(TEST_SUITE)\" -DEMBARC_UNIT_TEST

EXTRA_CSRCDIR += $(EMBARC_ROOT)/tests/arc $(EMBARC_ROOT)/tests/driver/uart  \
		 $(EMBARC_ROOT)/tests//library/console \
		 $(EMBARC_ROOT)/tests/board
EXTRA_ASMSRCDIR += $(EMBARC_ROOT)/tests/arc
# include current project makefile
COMMON_COMPILE_PREREQUISITES += makefile

### Options above must be added before include options.mk ###
# include key embARC build system makefile
include $(EMBARC_ROOT)/options/options.mk