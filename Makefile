CXX = gcc
EXEC = coquillage
 
CXXFLAGS = -Wall -Werror -std=c99
LDFLAGS =

OBJ = coquillage.o process_environment.o
 
all: coquillage
 
coquillage: $(OBJ)
	@$(CXX) -o $@ $^ $(LDFLAGS)
 
%.o: %.c
	@$(CXX) -o $@ -c $< $(CXXFLAGS)

h:
	@$(CXX) -o $@ helloworld.c $(CXXFLAGS)

c:
	@$(CXX) -o $@ notre_cat.c $(CXXFLAGS)

codify:
	@$(CXX) -o $@ codify.c $(CXXFLAGS)

clean:
	@rm -f *.o
	@rm -f coquillage h c codify
