CPP_FILES := $(wildcard ./*.cpp)
OBJ_FILES := $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS := -std=c++11
CC_FLAGS +=	-Wall -Wextra -pedantic-errors -fopenmp
CC_FLAGS += -O3
LD_LIBS := -fopenmp
EXECUTABLE := Localization

$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)
	
%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.d $(EXECUTABLE)

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)	