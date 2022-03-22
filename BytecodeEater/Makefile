LDFLAGS := -lfmt

DIR := ./parser ./scanner ./compiler ./debugger ./vm
ALLSRCS := $(foreach dir, $(DIR), $(wildcard $(dir)/*.cc))
MAINSRCS := $(foreach dir, $(DIR), $(wildcard $(dir)/main.cc))
SRCS := $(filter-out $(MAINSRCS), $(ALLSRCS))
OBJS := $(patsubst %.cc, %.o, $(SRCS))

TESTDIR := ./vm ./debugger
TESTSRC := $(foreach dir, $(TESTDIR), $(wildcard $(dir)/*.cc))
TESTOBJ := $(patsubst %.cc, %.o, $(TESTSRC))
show : 
	@echo $(SRCS)
.PHONY: scanner parser interpreter resolver vm debugger compiler

all: scanner parser compiler debugger vm 
	$(CXX) $(OBJS) -g ./scanner/main.o -o lox -lfmt
test: vm debugger
	$(CXX) $(TESTOBJ) -o tmain  -lfmt

scanner : 
	$(MAKE) $@ -C ./scanner 
parser : 
	$(MAKE) $@ -C ./parser
interpreter :
	$(MAKE) $@ -C ./interpreter
resolver :
	$(MAKE) $@ -C ./resolver
vm :
	$(MAKE) $@ -C ./vm
debugger :	
	$(MAKE) $@ -C ./debugger 
compiler :
	$(MAKE) $@ -C ./compiler

.PHONY: clean clean_test 

clean :
	$(MAKE) $@ -C ./scanner 
	$(MAKE) $@ -C ./parser
	$(MAKE) $@ -C ./compiler
	$(MAKE) $@ -C ./debugger
	$(MAKE) $@ -C ./vm

clean_test :
	$(MAKE) clean -C ./vm
	$(MAKE) clean -C ./debugger 




