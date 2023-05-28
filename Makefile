
# Avoid builtin rules and variables
MAKEFLAGS += -rR

PROG = multi-sim
all: $(PROG)


# Compilation tools
MPICPP := mpic++


# Compilation flags

_cppflags :=
_cppflags += -Wall
_cppflags += -MMD -MP		# Generate dependency files
_cppflags += -I /usr/include/eigen3
_cppflags += -fopenmp

_ldflags := -lgomp
_ldflags += -lceres
_ldflags += -lglog

CPPFLAGS := $(strip $(_cppflags))
LDFLAGS := $(strip $(_ldflags))


# Object files
_objs := multi-sim.o

OBJS := $(strip $(_objs))
DEPS := $(patsubst %.o,%.d,$(OBJS))


# Compilation rules
## Don't print the commands unless explicitely requested with `make V=1`
ifneq ($(V),1)
Q = @
endif

${PROG}: ${OBJS}
	@echo "MPICPP	$(notdir $@)"
	$(Q)$(MPICPP) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): %.o: %.cpp
	@echo "MPICPP	$(notdir $@)"
	$(Q)$(MPICPP) -c $(CPPFLAGS) -o $@ $<


## Cleaning rules
clean: FORCE
	@echo "CLEAN"
	$(Q)rm -f $(PROG) $(OBJS) $(DEPS)


## Rules configuration
FORCE:
.PHONY: FORCE

## Include dependencies
-include $(wildcard *.d)
