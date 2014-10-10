CXX = gcc
EXEC = coquillage
 
CXXFLAGS = -Wall -Werror -std=c99
LDFLAGS =

OBJ = coquillage.o process_environment.o
 
all: coquillage commands
 
coquillage: $(OBJ)
	@$(CXX) -o $@ $^ $(LDFLAGS)
 
%.o: %.c
	@$(CXX) -o $@ -c $< $(CXXFLAGS)

commands:
	@$(CXX) -o codify commands-src/codify.c $(CXXFLAGS)
	@$(CXX) -o c commands-src/notre_cat.c $(CXXFLAGS)
	@$(CXX) -o h commands-src/helloworld.c $(CXXFLAGS)

clean:
	@rm -f *.o
	@rm -f coquillage h c codify
