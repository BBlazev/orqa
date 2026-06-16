CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O2

TARGET   := program
SRCS     := $(wildcard *.cpp)
OBJS     := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
