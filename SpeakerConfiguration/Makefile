CPP_FILES := $(wildcard ./*.cpp)
OBJ_FILES := $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS := -std=c++11
CC_FLAGS +=	-Wall -Wextra -pedantic-errors
CC_FLAGS += -O3 -I../src
LD_LIBS := -lpthread -lssh -lssh_threads
EXECUTABLE := SpeakerConfiguration

$(EXECUTABLE): $(OBJ_FILES) Connections.o SSH.o
	g++ $(LD_FLAGS) -o $@ $^ $(LD_LIBS)

Connections.o: ../src/Connections.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
SSH.o: ../src/SSH.cpp
	g++ $(CC_FLAGS) -c -o $@ $<

%.o: %.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.d $(EXECUTABLE)

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)