SRC := $(shell find . -name '*.cpp') $(shell find . -name '*.c')
EXCLUDE_SRC := 
FSRC := $(filter-out $(EXCLUDE_SRC), $(SRC))
OBJ := $(FSRC:%=%.o)

DEP_DIR = .deps

FLAGS ?= -Wall -Wno-unknown-pragmas -I ../ttl/ttl/include -I ../date/include -I ../grpcbackend_cpp/
CXXFLAGS ?= -std=c++14 -DPICOJSON_USE_INT64
CFLAGS ?=
LINKFLAGS ?= -L../grpcbackend_cpp/debug/static -lgrpcbackend -lprotobuf -lgrpc++ -lgpr -lboost_system -lboost_filesystem -pthread -ldl -lz

OUTNAME := example

.PHONY: clean debug release

debug: FLAGS += -g -O0 -fPIC -fsanitize=address -fno-omit-frame-pointer
debug: LINKFLAGS := -lasan $(LINKFLAGS)
debug: $(OUTNAME)

release: FLAGS += -O2 -fPIC -march=native -flto
release: $(OUTNAME)
	@strip --strip-unneeded $(OUTNAME)

$(OUTNAME): $(OBJ)
	@echo Generating $@
	@$(CXX) -o $@ $^ $(LINKFLAGS)
	@echo Build done

%.cc.o: %.cc
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.cpp.o: %.cpp
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.c.o: %.c
	@echo Building $<
	@mkdir -p `dirname $@`
	@$(CC) -c $(FLAGS) $(CFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CC) -c $(FLAGS) $(CFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

clean:
	@echo Removing build results
	@rm -rf $(OUTNAME) $(OBJ)
	@echo Removing dependency files
	@rm -rf $(DEP_DIR)
	@echo Removing deb files
	@rm -rf $(DEBFOLDER) $(DEBFOLDER).deb

-include $(OBJ:%=$(DEP_DIR)/%.d)
