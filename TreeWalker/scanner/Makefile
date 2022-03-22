CXXFLAGS := -std=c++11 -m64 -g
DDIR := ./depend
SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.cc, $(DDIR)/%.d, $(SRCS))
INC := $(wildcard $(DDIR)/*.d)
include $(INC)
main : $(OBJS)
	$(CXX) $^ -o $@

.PHONY: scanner
scanner : $(OBJS)

#第一次编译的时候所有源文件都会被编译，这个时候依赖文件也会生成
#到了后续的编译的时候就会根据依赖文件来判断是否进行更新编译
$(OBJS) : %.o : %.cc $(DDIR)/%.d
	@echo $^
	$(CXX) $(CXXFLAGS)  -c $< -o $@ 
$(DEPS) : $(DDIR)/%.d : %.cc
	$(CXX) $(CXXFLAGS) -MM $< -MF $@
  
.PHONY: clean

clean : 
	rm -f *.o 
	rm -f ./depend/*.d 
