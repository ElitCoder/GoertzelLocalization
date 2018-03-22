CPP_FILES	:= $(wildcard ./*.cpp)
OBJ_FILES	:= $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))

CC_FLAGS 	:= -std=c++11 -Wall -Wextra -pedantic-errors
CC_FLAGS	+= -O3
CC_FLAGS	+= -fopenmp
LD_LIBS		:= -fopenmp

EXECUTABLE	:= Localization3D

$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)
	
%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.d $(EXECUTABLE) server_placements_results.txt live_localization.txt

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)	