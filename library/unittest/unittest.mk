# directory declaration
LIB_UNITTEST_DIR = $(LIBRARIES_ROOT)/unittest

LIB_UNITTEST_ASMSRCDIR	= $(LIB_UNITTEST_DIR)
LIB_UNITTEST_CSRCDIR	= $(LIB_UNITTEST_DIR)
LIB_UNITTEST_INCDIR	= #$(LIB_UNITTEST_DIR)

# find all the source files in the target directories
LIB_UNITTEST_CSRCS = $(call get_csrcs, $(LIB_UNITTEST_CSRCDIR))
LIB_UNITTEST_ASMSRCS = $(call get_asmsrcs, $(LIB_UNITTEST_ASMSRCDIR))

# get object files
LIB_UNITTEST_COBJS = $(call get_relobjs, $(LIB_UNITTEST_CSRCS))
LIB_UNITTEST_ASMOBJS = $(call get_relobjs, $(LIB_UNITTEST_ASMSRCS))
LIB_UNITTEST_OBJS = $(LIB_UNITTEST_COBJS) $(LIB_UNITTEST_ASMOBJS)

# get dependency files
LIB_UNITTEST_DEPS = $(call get_deps, $(LIB_UNITTEST_OBJS))

# extra macros to be defined
LIB_UNITTEST_DEFINES = -DLIB_UNITTEST

# genearte library
LIB_UNITTEST = $(OUT_DIR)/libembarc_unitest.a


# library generation rule
$(LIB_UNITTEST): $(LIB_UNITTEST_OBJS)
	$(TRACE_ARCHIVE)
	$(Q)$(AR) $(AR_OPT) $@ $(LIB_UNITTEST_OBJS)

# specific compile rules
# user can add rules to compile this middleware
# if not rules specified to this middleware, it will use default compiling rules

# Middleware Definitions
LIB_INCDIR += $(LIB_UNITTEST_INCDIR)
LIB_CSRCDIR += $(LIB_UNITTEST_CSRCDIR)
LIB_ASMSRCDIR += $(LIB_UNITTEST_ASMSRCDIR)

LIB_CSRCS += $(LIB_UNITTEST_CSRCS)
LIB_CXXSRCS +=
LIB_ASMSRCS += $(LIB_UNITTEST_ASMSRCS)
LIB_ALLSRCS += $(LIB_UNITTEST_CSRCS) $(LIB_UNITTEST_ASMSRCS)

LIB_COBJS += $(LIB_UNITTEST_COBJS)
LIB_CXXOBJS +=
LIB_ASMOBJS += $(LIB_UNITTEST_ASMOBJS)
LIB_ALLOBJS += $(LIB_UNITTEST_OBJS)

LIB_DEFINES += $(LIB_UNITTEST_DEFINES)
LIB_DEPS += $(LIB_UNITTEST_DEPS)
LIB_LIBS += $(LIB_UNITTEST)