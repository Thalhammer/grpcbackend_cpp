PROTOSRC = $(shell find . -name '*.proto')
PROTOHEADERS = $(PROTOSRC:.proto=.grpc.pb.h) $(PROTOSRC:.proto=.pb.h)
PROTOGEN = $(PROTOHEADERS:.h=.cc)
SRC = $(shell find . -name '*.cpp') $(shell find . -name '*.c') $(shell find . -name '*.cc') $(PROTOSRC:.proto=.grpc.pb.cc) $(PROTOSRC:.proto=.pb.cc)
EXCLUDE_SRC =
FSRC = $(filter-out $(EXCLUDE_SRC), $(SRC))
OBJ = $(FSRC:=.o)

DEP_DIR = .deps

FLAGS = -Wall -Wno-unknown-pragmas -fstack-protector-strong -flto
CXXFLAGS = -std=c++14
CFLAGS =
LINKFLAGS = -lprotobuf -lgrpc++ -lgrpc -pthread

OUTFILE = debug_client

.PHONY: clean debug release
.PRECIOUS: $(PROTOGEN) $(PROTOHEADERS)

release: FLAGS += -O2
release: $(OUTFILE)

debug: FLAGS += -g -O0
debug: $(OUTFILE)

$(OUTFILE): $(OBJ)
	@echo Generating binary
	@$(CXX) -o $@ $^ $(LINKFLAGS)
	@echo Build done

%.cpp.o: %.cpp $(PROTOHEADERS)
	@echo Building $<
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.cc.o: %.cc $(PROTOHEADERS)
	@echo Building $<
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CXX) -c $(FLAGS) $(CXXFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

%.c.o: %.c $(PROTOHEADERS)
	@echo Building $<
	@$(CC) -c $(FLAGS) $(CFLAGS) $< -o $@
	@mkdir -p `dirname $(DEP_DIR)/$@.d`
	@$(CC) -c $(FLAGS) $(CFLAGS) -MT '$@' -MM $< > $(DEP_DIR)/$@.d

clean:
	@echo Removing app
	@rm -f $(OUTFILE)
	@echo Removing objects
	@rm -f $(OBJ)
	@echo Removing dependency files
	@rm -rf $(DEP_DIR)
	@echo Removing Protobuf generated files
	@rm -rf $(PROTOHEADERS) $(PROTOGEN)
	@echo Removing debian packages
	@rm -rf $(DEBFOLDER)
	@rm -f $(DEBFOLDER).deb


%.grpc.pb.cc %.grpc.pb.h: %.proto
	protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.pb.cc %.pb.h: %.proto
	protoc -I . --cpp_out=. $<

-include $(OBJ:%=$(DEP_DIR)/%.d)