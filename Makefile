CXX = gcc
EXEC = coquillage

CXXFLAGS = -Wall -Werror -std=c99
LDFLAGS =

OBJ = coquillage.o process_management.o

all: coquillage commands

coquillage: $(OBJ)
	@$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@$(CXX) -o $@ -c $< $(CXXFLAGS)

commands:
	@$(CXX) -o codify commands-src/codify.c $(CXXFLAGS)
	@$(CXX) -o cat commands-src/notre_cat.c $(CXXFLAGS)
	@$(CXX) -o helloworld commands-src/helloworld.c $(CXXFLAGS)
	@$(CXX) -o echo commands-src/notre_echo.c $(CXXFLAGS)

clean:
	@rm -f *.o
	@rm -f coquillage helloworld cat codify echo
