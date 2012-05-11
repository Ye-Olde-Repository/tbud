
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "shared.h"
#include "entity.h"



// Acessories						
Entity* find_object(char* text, VEC_ENT&);					// search for object
Entity* find_object(unsigned eid, VEC_ENT&);					// search by only id
bool login(const char* user, const char* pass);
void Globalize(Entity *ent);						// completely add entity to game
void trim_input(char* string, int len);				// trim input from telnet
void clean_input (char* string, int len);			// remove invalid chars from telnet input
bool eos(char* string);							// is stream input finished?

// global identification
unsigned	getnextid();	
void		releaseid(unsigned id);
void		reserveid(unsigned id);

// Entity modifiers
int set(Entity* ent, char* flag, char* setting);	// set flags
int parse_string (char** &paramv, const char* string);
int link (Entity* a, Entity* b);
int unlink (Entity* a, Entity* b);

// Debug
int Debug_DumpEntity (Entity*, char*);
void Debug_CycleVector (VEC_ENT& vec);

// Game
int InitGame();
void ShutdownGame();
int motd(Entity* ent);
int login_welcome(Entity* ent);

// Storage
int writegame();
int readgame();

// Database
  // searches entity database for a value within specified fieldname
  // returns with offset of first entity found -after- param specified offset
  int FindDBEntity(const unsigned int offset,
                    const char* field_name, const char* value);

// scan a vector for T and return correlating iterator if existant
template <class T> std::vector<T>::iterator findit(T& object, std::vector<T>& vec)																	
{
	std::vector<T>::iterator it;

	for (it=vec.begin(); it != vec.end(); it++)
	{
		if ((*it) == object)
			return it;
	}

	return vec.end();
}








#endif
