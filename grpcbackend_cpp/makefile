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
DEBVERSION := $(shell git describe --tags --long | cut -d- -f1,2 | cut -c2-)
DEBFOLDER := $(OUTNAME)_$(DEBVERSION)

.PHONY: clean debug release package debug_static debug_shared release_static release_shared
.PRECIOUS: $(PROTOGEN) $(PROTOHEADERS)

all: release debug sample
release: release_static release_shared
debug: debug_static debug_shared

sample: main.cpp debug_static
	@echo Generating $@
	$(CXX) $(FLAGS) $(CXXFLAGS) -fsanitize=address -fno-omit-frame-pointer -g -o $@ main.cpp -L./debug/static/ -lgrpcbackend $(LINKFLAGS) -lasan
	@echo Build done

debug_static: FLAGS += -g -O0 -fsanitize=address -fno-omit-frame-pointer
debug_static: LINKFLAGS += -lasan
debug_static:
	$(eval BUILDDIR=debug/static)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).a --no-print-directory
release_static: FLAGS += -O2 -march=native
release_static:
	$(eval BUILDDIR=release/static)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).a --no-print-directory
	@strip --strip-unneeded $(BUILDDIR)/$(OUTNAME).a
debug_shared: FLAGS += -g -O0 -fPIC -fsanitize=address -fno-omit-frame-pointer
debug_shared: LINKFLAGS += -lasan
debug_shared:
	$(eval BUILDDIR=debug/shared)
	$(eval export FLAGS)
	$(eval export BUILDDIR)
	@+make $(BUILDDIR)/$(OUTNAME).so --no-print-directory
release_shared: FLAGS += -O2 -fPIC -march=native -flto
release_shared:
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

$(BUILDDIR)/%.cc.o: %.cc $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.cpp.o: %.cpp $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

$(BUILDDIR)/%.c.o: %.c $(PROTOHEADERS)
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CC) -c $(FLAGS) $(CFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CC) -c $(FLAGS) $(CFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.grpc.pb.cc %.grpc.pb.h: %.proto
	@echo Generating $^
	@protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.pb.cc %.pb.h: %.proto
	@echo Generating $^
	@protoc -I . --cpp_out=. $<

clean:
	@echo Removing build results
	@rm -rf debug release
	@echo Removing dependency files
	@rm -rf $(DEP_DIR)
	@echo Removing deb files
	@rm -rf $(DEBFOLDER) $(DEBFOLDER).deb

package: $(DEBFOLDER).deb

$(DEBFOLDER).deb: | release
	@rm -r -f $(DEBFOLDER)
	@echo Creating package
	@mkdir -p $(DEBFOLDER)/DEBIAN
	@mkdir -p $(DEBFOLDER)/usr/include/grpcbackend/
	@mkdir -p $(DEBFOLDER)/usr/lib/
	@echo "Package: libgrpcbackend" >> $(DEBFOLDER)/DEBIAN/control
	@echo "Version: $(DEBVERSION)" >> $(DEBFOLDER)/DEBIAN/control
	@echo "Section: devel" >> $(DEBFOLDER)/DEBIAN/control
	@echo "Priority: optional" >> $(DEBFOLDER)/DEBIAN/control
ifeq ($(ARCH),64)
	@echo "Architecture: amd64" >> $(DEBFOLDER)/DEBIAN/control
else
	@echo "Architecture: i386" >> $(DEBFOLDER)/DEBIAN/control
endif
	@echo "Depends: libssl1.0.0, libstdc++6, libgcc1" >> $(DEBFOLDER)/DEBIAN/control
	@echo "Maintainer: Dominik Thalhammer <dominik@thalhammer.it>" >> $(DEBFOLDER)/DEBIAN/control
	@echo "Description: Provides a library to help developing a service for mod_grpcbackend." >> $(DEBFOLDER)/DEBIAN/control
	@find . -name $(DEBFOLDER) -prune -o -name '*.h' -exec cp --parents \{\} $(DEBFOLDER)/usr/include/grpcbackend/ \;
	@cp release/shared/$(OUTNAME).so $(DEBFOLDER)/usr/lib/$(OUTNAME).so.$(DEBVERSION)
	@cp release/static/$(OUTNAME).a $(DEBFOLDER)/usr/lib/$(OUTNAME).a
	@ln -s $(OUTNAME).so.$(DEBVERSION) $(DEBFOLDER)/usr/lib/$(OUTNAME).so
	@fakeroot dpkg-deb --build $(DEBFOLDER)

-include $(OBJ:%=$(DEP_DIR)/%.d)
