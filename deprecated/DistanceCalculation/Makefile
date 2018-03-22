CPP_FILES	:= $(wildcard src/*.cpp)
OBJ_FILES	:= $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

CC_FLAGS	:= -std=c++11 -Wall -Wextra -pedantic-errors -fopenmp
CC_FLAGS	+= -O3
LD_LIBS		:= -lnessh -fopenmp

EXECUTABLE := DistanceCalculation

bin/$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)

obj/%.o: src/%.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -f obj/* bin/$(EXECUTABLE)
	rm -rf bin/scripts/
	rm -rf bin/recordings/
	rm -f Calculated\ level\ 3
	rm -f bin/server_results.txt
	
CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)