PROTOSRC := $(shell find . -name '*.proto')
PROTOHEADERS := $(PROTOSRC:.proto=.grpc.pb.h) $(PROTOSRC:.proto=.pb.h)
PROTOGEN := $(PROTOHEADERS:.h=.cc)
SRC := $(shell find . -name '*.cpp') $(shell find . -name '*.c') $(PROTOSRC:.proto=.grpc.pb.cc) $(PROTOSRC:.proto=.pb.cc)
EXCLUDE_SRC := ./main.cpp
FSRC := $(filter-out $(EXCLUDE_SRC), $(SRC))
OBJ := $(FSRC:%=$(BUILDDIR)/%.o)

DEP_DIR = .deps

FLAGS ?= -Wall -Wno-unknown-pragmas -I ../ttl/ttl/include -I ../date/include
CXXFLAGS ?= -std=c++14 -DPICOJSON_USE_INT64
CFLAGS ?=
LINKFLAGS ?= -lprotobuf -lgrpc++ -lboost_system -lboost_filesystem -pthread

OUTNAME := libgrpcbackend

ARCH := $(shell getconf LONG_BIT)
GITVERSION := $(shell git describe --tags --long | cut -d- -f1,2 | cut -c2-)
DEBVERSION ?= $(firstword $(subst -, ,$(GITVERSION)))
DEBITERATION ?= $(or $(word 2,$(subst -, ,$(GITVERSION))),0)
DEBFOLDER ?= $(OUTNAME)_$(DEBVERSION)-$(DEBITERATION)

FPM := fpm --verbose -s dir -t deb -v $(DEBVERSION) --iteration $(DEBITERATION) -m 'Dominik Thalhammer <dominik@thalhammer.it>' --vendor 'Dominik Thalhammer <dominik@thalhammer.it>' --url 'https://github.com/Thalhammer/grpcbackend_cpp' -C $(DEBFOLDER)

.PHONY: clean debug release package debug_static debug_shared release_static release_shared
.PRECIOUS: $(PROTOGEN) $(PROTOHEADERS)

all: release debug | $(PROTOHEADERS)
release: release_static release_shared | $(PROTOHEADERS)
debug: debug_static debug_shared | $(PROTOHEADERS)

debug_static: FLAGS += -g -Og -fsanitize=address -fno-omit-frame-pointer
debug_static: LINKFLAGS += -lasan
debug_static: | $(PROTOHEADERS)
	$(eval BUILDDIR=debug/static)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).a --no-print-directory
release_static: FLAGS += -O2 -march=native
release_static: | $(PROTOHEADERS)
	$(eval BUILDDIR=release/static)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).a --no-print-directory
	@strip --strip-unneeded $(BUILDDIR)/$(OUTNAME).a
debug_shared: FLAGS += -g -Og -fPIC -fsanitize=address -fno-omit-frame-pointer
debug_shared: LINKFLAGS += -lasan
debug_shared: | $(PROTOHEADERS)
	$(eval BUILDDIR=debug/shared)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).so --no-print-directory
release_shared: FLAGS += -O2 -fPIC -march=native -flto
release_shared: | $(PROTOHEADERS)
	$(eval BUILDDIR=release/shared)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).so --no-print-directory
	@strip --strip-unneeded $(BUILDDIR)/$(OUTNAME).so

$(BUILDDIR)/$(OUTNAME).so: $(OBJ)
	@echo Generating $@
	@$(CXX) -shared -o $@ $^ $(LINKFLAGS)
	@echo Build done

$(BUILDDIR)/$(OUTNAME).a: $(OBJ)
	@echo Generating $@
	@gcc-ar rcs $@ $^
	@echo Build done

$(BUILDDIR)/%.cc.o: %.cc  | $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.cpp.o: %.cpp | $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.c.o: %.c | $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CC) -c $(FLAGS) $(CFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CC) -c $(FLAGS) $(CFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.grpc.pb.cc %.grpc.pb.h: %.proto
	@echo Generating $@
	@protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.pb.cc %.pb.h: %.proto
	@echo Generating $@
	@protoc -I . --cpp_out=. $<

clean:
	@echo Removing build results
	@rm -rf debug release
	@echo Removing dependency files
	@rm -rf $(DEP_DIR)
	@echo Removing deb files
	@rm -rf $(DEBFOLDER) libgrpcbackend_$(DEBVERSION)-$(DEBITERATION)_amd64.deb libgrpcbackend0_$(DEBVERSION)-$(DEBITERATION)_amd64.deb libgrpcbackend-dev_$(DEBVERSION)-$(DEBITERATION)_amd64.deb

distclean: clean
	@echo Removing generated protobuf files
	@rm -rf $(PROTOGEN) $(PROTOHEADERS)

package: | release
	@rm -rf $(DEBFOLDER)
	@echo Creating package
	@mkdir $(DEBFOLDER)
	# Build legacy package with no contents
	@$(FPM) -n libgrpcbackend --description "libgrpcbackend-dev legacy upgrade package" -d "libgrpcbackend-dev"

	@mkdir -p $(DEBFOLDER)/usr/include/grpcbackend/
	@mkdir -p $(DEBFOLDER)/usr/lib/

	@find . -name $(DEBFOLDER) -prune -o -name '*.h' -exec cp --parents \{\} $(DEBFOLDER)/usr/include/grpcbackend/ \;
	@cp release/shared/$(OUTNAME).so $(DEBFOLDER)/usr/lib/$(OUTNAME).so.$(DEBVERSION)
	@cp release/static/$(OUTNAME).a $(DEBFOLDER)/usr/lib/$(OUTNAME).a
	@ln -s $(OUTNAME).so.$(DEBVERSION) $(DEBFOLDER)/usr/lib/$(OUTNAME).so
	@$(FPM) -n libgrpcbackend0 --description "Provides a library to help developing a service for mod_grpcbackend." -d "libssl1.0.0" -d "libstdc++6" -d "libgcc1" usr/lib/$(OUTNAME).so.$(DEBVERSION)
	@$(FPM) -n libgrpcbackend-dev --description "Provides a library to help developing a service for mod_grpcbackend (development headers)" -d "libgrpcbackend0 = $(DEBVERSION)-$(DEBITERATION)" usr/include/grpcbackend usr/lib/$(OUTNAME).a usr/lib/$(OUTNAME).so

-include $(OBJ:%=$(DEP_DIR)/%.d)
