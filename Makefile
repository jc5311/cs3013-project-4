
all: proj4.o
	g++ proj4.o -o proj4

proj4.o: proj4.cpp
	g++ -c proj4.cpp

clean:
	rm *.o proj4