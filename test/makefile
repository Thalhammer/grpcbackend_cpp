SRC := $(shell find . -name '*.cpp') $(shell find . -name '*.c')
EXCLUDE_SRC := 
FSRC := $(filter-out $(EXCLUDE_SRC), $(SRC))
OBJ := $(FSRC:%=$(BUILDDIR)/%.o)

DEP_DIR = .deps

FLAGS ?= -Wall -Wno-unknown-pragmas -I ../ttl/ttl/include -I ../date/include -I ../grpcbackend_cpp
CXXFLAGS ?= -std=c++14 -DPICOJSON_USE_INT64
CFLAGS ?=
LINKFLAGS ?= -lprotobuf -lgrpc++ -lboost_system -lboost_filesystem -lz -pthread -lgtest -lgtest_main

OUTNAME := test

.PHONY: clean debug release

all: release debug

debug: FLAGS += -g -Og -fsanitize=address -fno-omit-frame-pointer
debug: LINKFLAGS := -lasan -L../grpcbackend_cpp/debug/static/ -lgrpcbackend $(LINKFLAGS)
debug: | ../grpcbackend_cpp/debug/static/libgrpcbackend.a
	$(eval BUILDDIR=debug)
	$(eval export FLAGS)
	$(eval export LINKFLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME) --no-print-directory
release: FLAGS += -O2 -march=native
release: LINKFLAGS := -L../grpcbackend_cpp/release/static/ -lgrpcbackend $(LINKFLAGS)
release: | ../grpcbackend_cpp/release/static/libgrpcbackend.a
	$(eval BUILDDIR=release)
	$(eval export FLAGS)
	$(eval export LINKFLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME) --no-print-directory

$(BUILDDIR)/$(OUTNAME): $(OBJ)
	@echo Generating $@
	@$(CXX) -o $@ $^ $(LINKFLAGS)
	@echo Build done

$(BUILDDIR)/%.cc.o: %.cc
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.cpp.o: %.cpp
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.c.o: %.c
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CC) -c $(FLAGS) $(CFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CC) -c $(FLAGS) $(CFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

clean:
	@echo Removing build results
	@rm -rf debug release
	@echo Removing dependency files
	@rm -rf $(DEP_DIR)

../grpcbackend_cpp/debug/static/libgrpcbackend.a:
	@+make -C ../grpcbackend_cpp debug_static

../grpcbackend_cpp/release/static/libgrpcbackend.a:
	@+make -C ../grpcbackend_cpp release_static

-include $(OBJ:%=$(DEP_DIR)/%.d)
