CXX = gcc
EXEC = coquillage
 
CXXFLAGS = -Wall
LDFLAGS =

OBJ = coquillage.o process_environment.o
 
all: coquillage
 
coquillage: $(OBJ)
	@$(CXX) -o $@ $^ $(LDFLAGS)
 
%.o: %.$(EXT)
	@$(CXX) -o $@ -c $< $(CXXFLAGS)
 
clean:
	@rm -f *.o
	@rm -f coquillage
