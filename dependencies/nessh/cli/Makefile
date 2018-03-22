CXX			:= g++

CPP_FILES	:= $(wildcard src/*.cpp)
OBJ_FILES	:= $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

CC_FLAGS	:= -std=c++11
CC_FLAGS	+= -Wall -Wextra -pedantic-errors
CC_FLAGS	+= -O3
LD_LIBS		:= -lnessh

EXECUTABLE	:= nessh-cli

all: build

install:
	chmod 755 nessh-cli
	cp nessh-cli /usr/bin/
	
clean:
	rm -f obj/* $(EXECUTABLE) stdout*

build: $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) $^ -o $(EXECUTABLE) $(LD_LIBS)

obj/%.o: src/%.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $<
	
CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)