# ayuda obtenida de https://www.slothparadise.com/c-libraries-linking-compiling/
INC = -I.

executable: fatreader.o fathelper.o fatentry.o
	g++ fatreader.o fathelper.o fatentry.o

fatreader.o: fatreader.cpp fathelper.h
	g++ $(INC) -c fatreader.cpp

fathelper.o: fathelper.cpp fathelper.h
	g++ $(INC) -c fathelper.cpp

fatentry.o: fatentry.cpp fatentry.h
	g++ $(INC) -c fatentry.cpp

clean:
	rm fatentry.o fathelper.o fatreader.o a.out