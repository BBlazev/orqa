CXX      := gcc
CXXFLAGS := -std=c11 -Wall -Wextra -Werror -pthread -O2

TARGET   := program
SRCS     := $(wildcard *.c)
OBJS     := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)



