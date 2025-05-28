CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -Ilib/SimpleTCPServerListener -g

SRCDIR = src
OBJDIR = obj

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

LIBDIR = lib/SimpleTCPServerListener
STATIC_LIB = $(LIBDIR)/libSimpleTcpServerListener.a
LIB_SRCDIR = $(LIBDIR)
LIB_SOURCES := $(wildcard $(LIB_SRCDIR)/*.cpp)

TARGET = ircserv

all: $(TARGET)

$(TARGET): $(STATIC_LIB) $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -L$(LIBDIR) -lSimpleTcpServerListener

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(STATIC_LIB): $(LIB_SOURCES)
	$(MAKE) -C $(LIBDIR)

clean:
	@echo "Cleaning project object files..."
	rm -rf $(OBJDIR)
	$(MAKE) -C $(LIBDIR) clean

fclean: clean
	@echo "Removing binary $(TARGET) and static library..."
	rm -f $(TARGET)
	$(MAKE) -C $(LIBDIR) fclean

re: fclean all

.PHONY: all clean fclean re