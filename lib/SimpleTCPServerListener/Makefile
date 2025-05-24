CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++98 -g -I.

SRCS = SimpleTcpServerListener.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = libSimpleTcpServerListener.a

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Creating static library $(TARGET)..."
	ar rcs $(TARGET) $(OBJS)

%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning library object files..."
	rm -f $(OBJS)

fclean: clean
	@echo "Removing $(TARGET)..."
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re
