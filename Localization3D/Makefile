CPP_FILES := $(wildcard ./*.cpp)
OBJ_FILES := $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS := -std=c++11
CC_FLAGS +=	-Wall -Wextra -pedantic-errors
CC_FLAGS += -O3
#LD_LIBS := -lpthread -lssh -lssh_threads
EXECUTABLE := Localization3D

$(EXECUTABLE): $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)
	
%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.d $(EXECUTABLE) live_localization.txt

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)	