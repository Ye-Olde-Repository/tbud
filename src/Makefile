tbud:	utility.o globals.o entity.o database.o network.o main.o
	g++ -Wall -o tbud utility.o globals.o entity.o database.o network.o main.o
 
utility.o:	utility.cpp utility.h
	g++ -Wall -o utility.o -c utility.cpp
  
globals.o:	globals.cpp globals.h
	g++ -Wall -o globals.o -c globals.cpp
  
entity.o:	entity.cpp entity.h
	g++ -Wall -o entity.o -c entity.cpp
  
database.o:	database.cpp database.h
	g++ -Wall -o database.o -c database.cpp
  
network.o:	network.cpp network.h
	g++ -Wall -o network.o -c network.cpp
  
main.o:	main.cpp shared.h
	g++ -Wall -o main.o -c main.cpp
