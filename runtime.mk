# Minimal Makefile to allow programs using this implementation to compile the same way as
# they do for the normal Legion. There is no support for USE_X flags, Fortran or GPUs.
# The only supported compilers are gcc on GNU/Linux and clang on macOS.

ifndef OUTFILE
$(error OUTFILE not defined, nothing to build)
endif

ifndef LG_RT_DIR
$(error LG_RT_DIR variable is not defined, aborting build)
endif

# These flags are used to control compiler/linker settings.
CFLAGS		?=
CC_FLAGS	?=
LD_FLAGS	?=
INC_FLAGS	?=

# Map some common GNU variable names into our variables.
CPPFLAGS	?=
CXXFLAGS	?=
LDFLAGS		?=
CC_FLAGS	+= $(CXXFLAGS) $(CPPFLAGS)
LD_FLAGS	+= $(LDFLAGS)

# This implementation is using some C++17 features.
CC_FLAGS	+= -std=c++17

# Machine architecture (generally "native" unless cross-compiling).
MARCH		?= native
CFLAGS		+= -march=${MARCH}
CC_FLAGS	+= -march=${MARCH}

# Include paths.
INC_FLAGS	+= -I$(LG_RT_DIR)

# Figure out OS.
ifeq ($(shell uname -s),Linux)
LINUX		= 1
endif
ifeq ($(shell uname -s),Darwin)
DARWIN		= 1
CFLAGS		+= -DDARWIN
CC_FLAGS	+= -DDARWIN
endif

# Support libraries (OS-specific).
ifeq ($(strip $(LINUX)),1)
LD_FLAGS	+= -lrt
endif

# Configure debug builds.
ifeq ($(strip $(DEBUG)),1)
  ifeq ($(strip $(DARWIN)),1)
    CFLAGS	+= -O0 -glldb
    CC_FLAGS	+= -O0 -glldb
  else
    CFLAGS	+= -O0 -ggdb
    CC_FLAGS	+= -O0 -ggdb
  endif
else
  CFLAGS	+= -O2
  CC_FLAGS	+= -O2 -fno-strict-aliasing
endif

# DEBUG_TSAN=1 enables thread sanitizer (data race) checks.
ifeq ($(strip $(DEBUG_TSAN)),1)
CFLAGS		+= -fsanitize=thread -g -DTSAN_ENABLED
CC_FLAGS	+= -fsanitize=thread -g -DTSAN_ENABLED
LD_FLAGS	+= -fsanitize=thread
endif

# DEBUG_ASAN=1 enables address sanitizer (memory error) checks.
ifeq ($(strip $(DEBUG_ASAN)),1)
CFLAGS		+= -fsanitize=address -g -DASAN_ENABLED
CC_FLAGS	+= -fsanitize=address -g -DASAN_ENABLED
LD_FLAGS	+= -fsanitize=address
endif

# Demand warning-free compilation.
CFLAGS		+= -Wall
CC_FLAGS	+= -Wall
ifeq ($(strip $(WARN_AS_ERROR)),1)
CC_FLAGS	+= -Werror
endif

# Gather source files.
CC_SRC		?=
CXX_SRC		?=
GEN_SRC		?=
CXX_SRC		+= $(GEN_SRC)

# Define object files.
APP_OBJS	:= $(CC_SRC:.c=.c.o)
APP_OBJS	+= $(CXX_SRC:.cc=.cc.o)
APP_OBJS	+= $(ASM_SRC:.S=.S.o)

# Provide build rules unless the user asks us not to.
ifndef NO_BUILD_RULES

# Provide an all unless the user asks us not to.
ifndef NO_BUILD_ALL
.PHONY : all
all : $(OUTFILE)
endif

$(OUTFILE) : $(APP_OBJS)
	@echo "---> Linking objects into one binary: $(OUTFILE)"
	$(CXX) -o $(OUTFILE) $(APP_OBJS) $(LD_FLAGS)

$(filter %.c.o,$(APP_OBJS)) : %.c.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(INC_FLAGS)

$(filter %.cc.o,$(APP_OBJS)) : %.cc.o : %.cc
	$(CXX) -o $@ -c $< $(CC_FLAGS) $(INC_FLAGS)

$(filter %.S.o,$(APP_OBJS)) : %.S.o : %.S
	$(AS) -o $@ -c $< $(CC_FLAGS) $(INC_FLAGS)

# Disable gmake's default rule for building % from %.o.
% : %.o

clean :
	$(RM) -f $(OUTFILE) $(APP_OBJS)

endif # NO_BUILD_RULES
