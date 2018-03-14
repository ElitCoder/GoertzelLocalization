CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS := -std=c++11
CC_FLAGS +=	-Wall -Wextra -pedantic-errors -fopenmp
CC_FLAGS += -O3
CC_FLAGS += -I./dep/prebuilt/include/
LD_FLAGS := -L./dep/prebuilt/lib/
LD_LIBS := -lpthread -lssh -lssh_threads -fopenmp
EXECUTABLE := GoertzelLocalization

bin/$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)

obj/%.o: src/%.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf obj/*.o obj/*.d bin/$(EXECUTABLE)
	rm -rf bin/scripts/
	rm -rf bin/recordings/
	rm -rf Calculated\ level\ 3
	
CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)
