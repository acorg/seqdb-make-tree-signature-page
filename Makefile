# -*- Makefile -*-
# Eugene Skepner 2016

# submodules and git: https://git-scm.com/book/en/v2/Git-Tools-Submodules

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

# TEST_SOURCES = test.cc seqdb.cc seqdb-json.cc read-file.cc xz.cc
SEQDB_SOURCES = seqdb.cc seqdb-py.cc amino-acids.cc clades.cc \
		tree.cc tree-import.cc newick.cc settings.cc chart.cc \
		draw.cc coloring.cc geographic-map.cc continent-map.cc \
		signature-page.cc draw-tree.cc time-series.cc draw-clades.cc antigenic-maps.cc

TEST_CAIRO_SOURCES = test-cairo.cc draw.cc

# ----------------------------------------------------------------------

CLANG = $(shell if g++ --version 2>&1 | grep -i llvm >/dev/null; then echo Y; else echo N; fi)
ifeq ($(CLANG),Y)
  WEVERYTHING = -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded
  WARNINGS = -Wno-weak-vtables # -Wno-padded
  STD = c++14
  CXXFLAGS_EXTRA =
else
  WEVERYTHING = -Wall -Wextra
  WARNINGS =
  STD = c++14
  CXXFLAGS_EXTRA = -ftemplate-depth=2000 # cc/seqdb-py.cc
endif

PYTHON_VERSION = $(shell python3 -c 'import sys; print("{0.major}.{0.minor}".format(sys.version_info))')
PYTHON_CONFIG = python$(PYTHON_VERSION)-config
PYTHON_MODULE_SUFFIX = $(shell $(PYTHON_CONFIG) --extension-suffix)

# -fvisibility=hidden and -flto make resulting lib smaller (pybind11) but linking is much slower
OPTIMIZATION = -O3 #-fvisibility=hidden -flto
CXXFLAGS = -MMD -g $(OPTIMIZATION) -fPIC -std=$(STD) $(WEVERYTHING) $(WARNINGS) -I$(BUILD)/include -I$(ACMACSD_ROOT)/include $(PKG_INCLUDES) $(MODULES_INCLUDE) $(CXXFLAGS_EXTRA)
LDFLAGS =
TEST_CAIRO_LDLIBS = $$(pkg-config --libs cairo)
SEQDB_LDLIBS = $$(pkg-config --libs cairo) $$(pkg-config --libs liblzma) $$($(PYTHON_CONFIG) --ldflags | sed -E 's/-Wl,-stack_size,[0-9]+//')

MODULES_INCLUDE = -Imodules/json/src -Imodules/axe/include -Imodules/pybind11/include -Imodules/json-struct
PKG_INCLUDES = $$(pkg-config --cflags cairo) $$(pkg-config --cflags liblzma) $$($(PYTHON_CONFIG) --includes)

# ----------------------------------------------------------------------

BUILD = build
DIST = dist

all: check-acmacsd-root $(DIST)/seqdb_backend$(PYTHON_MODULE_SUFFIX) $(DIST)/test-cairo

install: check-acmacsd-root $(DIST)/seqdb_backend$(PYTHON_MODULE_SUFFIX)

-include $(BUILD)/*.d

# ----------------------------------------------------------------------

$(DIST)/test-cairo: $(patsubst %.cc,$(BUILD)/%.o,$(TEST_CAIRO_SOURCES)) | $(DIST)
	g++ $(LDFLAGS) -o $@ $^ $(TEST_CAIRO_LDLIBS)

$(DIST)/seqdb_backend$(PYTHON_MODULE_SUFFIX): $(patsubst %.cc,$(BUILD)/%.o,$(SEQDB_SOURCES)) | $(DIST)
	g++ -shared $(LDFLAGS) -o $@ $^ $(SEQDB_LDLIBS)
	@#strip $@

$(DIST)/json-parser-test: $(BUILD)/json-parser-test.o | $(DIST)
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(DIST) $(BUILD)/*.o $(BUILD)/*.d $(BUILD)/submodules

distclean: clean
	rm -rf $(BUILD)

# ----------------------------------------------------------------------

$(BUILD)/%.o: cc/%.cc | $(BUILD) $(BUILD)/submodules
	@echo $<
	@g++ $(CXXFLAGS) -c -o $@ $<

# ----------------------------------------------------------------------

$(BUILD)/submodules:
	git submodule init
	git submodule update
	git submodule update --remote
	touch $@

# ----------------------------------------------------------------------

check-acmacsd-root:
ifndef ACMACSD_ROOT
	$(error ACMACSD_ROOT is not set)
endif

$(DIST):
	mkdir -p $(DIST)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: check-acmacsd-root

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
