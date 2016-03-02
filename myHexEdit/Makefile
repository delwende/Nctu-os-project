CC=g++
CFLAGS=-O2 --std=c++11 -Wall -Wshadow -Wno-unused-result

CFLAGS += -g

OUTPUT = myHexEdit
OBJ	=	myHexEdit

$(OUTPUT): $(addsuffix .o, $(OBJ))
	mkdir -p ../out/
	$(CC) -o ../out/$@ $^ $(CFLAGS)

%.o: ../%.c 
	$(CC) -c -o $@ $< $(CFLAGS)
	
clean:
	rm -f *.o
	rm -f ../*.o
	rm -r ../out/

cleanobj:
	rm -f *.o