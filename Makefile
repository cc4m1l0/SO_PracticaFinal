# ayuda obtenida de https://www.slothparadise.com/c-libraries-linking-compiling/
INC = -I.

executable: fatreader.o fathelper.o
	g++ fatreader.o fathelper.o

fatreader.o: fatreader.cpp fathelper.h
	g++ $(INC) -c fatreader.cpp

fathelper.o: fathelper.cpp fathelper.h
	g++ $(INC) -c fathelper.cpp

clean:
	rm fathelper.o fatreader.o a.out