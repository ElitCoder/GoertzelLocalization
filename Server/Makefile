CPP_FILES	:= $(wildcard ./*.cpp)
OBJ_FILES	:= $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS	:= -std=c++11 -Wall -Wextra -pedantic-errors
CC_FLAGS	+= -O3
LD_LIBS		:= -lnessh -lpthread

EXECUTABLE	:= Server

$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)

%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.d $(EXECUTABLE)
	rm -rf speaker_results

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)