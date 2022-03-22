DIR := ./parser ./scanner ./interpreter ./resolver
ALLSRCS := $(foreach dir, $(DIR), $(wildcard $(dir)/*.cc))
MAINSRCS := $(foreach dir, $(DIR), $(wildcard $(dir)/main.cc))
SRCS := $(filter-out $(MAINSRCS), $(ALLSRCS))
OBJS := $(patsubst %.cc, %.o, $(SRCS))
show : 
	@echo $(SRCS)
.PHONY: scanner parser interpreter resolver

all: scanner parser interpreter resolver
	$(CXX) $(OBJS) ./scanner/main.o -o lox 
scanner : 
	$(MAKE) $@ -C ./scanner 
parser : 
	$(MAKE) $@ -C ./parser
interpreter :
	$(MAKE) $@ -C ./interpreter
resolver :
	$(MAKE) $@ -C ./resolver

.PHONY: clean

clean :
	$(MAKE) $@ -C ./scanner 
	$(MAKE) $@ -C ./parser
	$(MAKE) $@ -C ./interpreter
	$(MAKE) $@ -C ./resolver





