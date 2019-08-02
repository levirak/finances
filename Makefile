program_name = $(notdir $(CURDIR))
source_dir   = src
build_dir    = build

o = .o
d = .mk
e =

CPPDIRS = -iquote $(build_dir)
LDDIRS  =
LDLIBS  =

added_flags       := $(CFLAGS)
override CPPFLAGS := $(CPPFLAGS) $(CPPDIRS)
override CFLAGS   := -g -std=gnu11 -Wall -Wextra -Wpedantic $(CFLAGS) -fsanitize=address,undefined,leak
override LDFLAGS  := $(LDFLAGS) $(LDDIRS)

sources = $(wildcard $(source_dir)/*.c)
objects = $(patsubst $(source_dir)/%.c,$(build_dir)/%$o,$(sources))
deps    = $(patsubst $(source_dir)/%.c,$(build_dir)/%$d,$(sources))
target  = $(build_dir)/$(program_name)$e

# The Target Build
all: $(target)

release: CFLAGS := -O2 -Wall -Wextra -DNDEBUG $(added_flags)
release: $(target)

-include $(deps)
$(deps): $(build_dir)/%$d: $(source_dir)/%.c
	@echo "  GEN   $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(CPPFLAGS) $< -MM -MT $(@:$d=$o) >$@

$(target): $(objects)
	@echo "  CC    $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(objects): %$o: # dependencies provided by deps
	@echo "  CC    $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< $(LDFLAGS) $(LDLIBS)

.PHONY: clean cleaner
clean:
	@echo "  RM    $(build_dir)"
	@rm -rf $(build_dir)

cleaner: clean
	@echo "  RM    tags"
	@rm -f tags

.PHONY: tags
tags:
	@echo "  CTAGS $(source_dir)"
	@ctags -R $(source_dir)
