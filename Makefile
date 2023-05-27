
# Avoid builtin rules and variables
MAKEFLAGS += -rR

PROGS = multi-sim
all: $(PROGS)


# Compilation tools
MPICPP := mpic++


# Compilation rules
## Don't print the commands unless explicitely requested with `make V=1`
ifneq ($(V),1)
Q = @
endif

multi-sim: main.cpp
	@echo "MPICPP	$(notdir $@)"
	$(Q)$(MPICPP) -o $@ $<


## Cleaning rules
clean: FORCE
	@echo "CLEAN"
	$(Q)rm -f $(PROGS)


## Rules configuration
FORCE:
.PHONY: FORCE

## Include dependencies
-include $(wildcard *.d)
