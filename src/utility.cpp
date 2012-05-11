
#include "shared.h"
#include "utility.h"
#include "entity.h"
#include "network.h"
#include "account.h"
#include "database.h"
#include "globals.h"

#define RENEW_DATAFILES true



// Displays motd file to specified client
int motd(Entity* ent)
{
	char buffer[MAX_BUFFER+1];
	memset(buffer,0,MAX_BUFFER+1);

	ifstream motd_file(MOTD_FILENAME, ios::in);
	if (motd_file.fail())
	{
		motd_file.close();
		return 0;
	}

	if (ent->Send("\n") == -1)
	{
		motd_file.close();
		return -1;
	}

	motd_file.getline(buffer, MAX_BUFFER);
	while (!motd_file.eof())
	{
		if (ent->Send("%s\n", buffer) == -1)
		{
			motd_file.close();
			return -1;
		}
		motd_file.getline(buffer, MAX_BUFFER);
	}

	motd_file.close();

	if (ent->Send("\n\n") == -1)
		return -1;
	

	return 1;
}

bool login(const char* u, const char* p)
{
  DB_FindEntity(
  
  
}


int login_welcome(Entity* ent)
{
	char buffer[MAX_BUFFER+1];
	memset(buffer,0,MAX_BUFFER+1);

	ifstream welcome_file(LOGINWELCOME_FILENAME, ios::in);
	if (welcome_file.fail())
	{
		welcome_file.close();
		return 0;
	}

	if (ent->Send("\n") == -1)
	{
		welcome_file.close();
		return -1;
	}

	welcome_file.getline(buffer, MAX_BUFFER);
	while (!welcome_file.eof())
	{
		if (ent->Send("%s\n", buffer) == -1)
		{
			welcome_file.close();
			return -1;
		}
		welcome_file.getline(buffer, MAX_BUFFER);
	}

	welcome_file.close();

	return 1;
}


// Initializes game and loads database.
// will print any errors that may occur and, by database routine, will
// try to correct them.
int InitGame()
{
	ENT_IT entIt;
	Entity* ent;

	/* Temporary Intro */
	cout << "Welcome to TBUD MUD!\n - Copyright Roy Laurie 1999\n\n";
	cout << "Current Build: " << TBUD_VER << "      NetCode: Online.\n";
	cout << "StorageCode: Available    DynamiCode: Available\n" << endl;

	game_flags |= GAMEFLG_INIT;
	memset(&id_list, 0, sizeof(id_list));
//	g_entIt = NULL;

	if (RENEW_DATAFILES) {
		system("del logins.dat");
		system("del entities.dat");
	}

	cout << "Loading entities...";
	
	if (!readgame())
	{
		cout << "unable to load game data base!" << endl;
		cout << "Loading hard-coded entities...";
	
		// Spawn a God, Master Room, and a temporary NPC for good measure.
		Globalize(new Entity("Master Room", "This area is the center of the entire MUD.", area));

		// God is playable character, don't allow it to spawn into active game.
		ent = new Entity("God", "The supreme entity of this world. His greatness is terrifying.", player);
		ent->hp = 1000;
		ent->max_hp = 1000;
		ent->flags |= EFL_WARP;
		reserveid(ent->id);
		ghosts.push_back(ent);

		Globalize(new Entity("Troll", "An ugly, evil NPC."));
		
		//  Write new database
		DB_Entities_UpdateAll();
	}

	// DEBUG: Check status of entities.
	ent = globals[INDX_HOME];
	for (entIt=ent->inventory.begin(); entIt != ent->inventory.end(); entIt++)
		1;

	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
		1;

	// Link and Init Globals
	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
	{
		if(!(*entIt)->InitData())
		{
			delete (*entIt);
			entIt = globals.erase(entIt);
			entIt--;
		}
	}

	cout << "done." << endl;

	// DEBUG: Check status of entities.
	ent = globals[INDX_HOME];
	for (entIt=ent->inventory.begin(); entIt != ent->inventory.end(); entIt++)
		1;

	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
		1;



	if (!DB_Logins_Begin())
		return 0;

	DB_Logins_End();

	cout << "Initial Size: " << globals.size() << endl << endl;
	game_flags &= ~GAMEFLG_INIT;

	return 1;
}



// Closes game properly.
// Saves the game-state, destroys all entities, etc.
void ShutdownGame()
{
	ENT_IT entIt;

	game_flags |= GAMEFLG_SHUTDOWN;
	cout << "Server SHUTDOWN..." << endl;
		
	closesocket(listen_sock);
	
	// Save game-state
	cout << "Saving game-state...";
	if (!DB_Entities_UpdateAll())	// removes all ghosts, no need to delete them later.
		cout << "failed!" << endl;
	else
		cout << "done." << endl;

	// Destroy globals
	cout << "Destroying entities...";
	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
	{
		delete (*entIt);
		entIt = globals.erase(entIt);
		entIt--;
	}

	cout << "done." << endl;

	cout << "TBUD shutdown procedure complete!\n" << endl;

	exit(1);
}




// Find an object within globals using raw text.
// For use mostly with client input. Searches for both id (first) and actual names (second).
// Returns NULL if not found.
Entity* find_object(char* text, VEC_ENT& vec)
{
	// Search by ID
	unsigned x = (unsigned)atoi(text);
	ENT_IT entIt;

	for (entIt=vec.begin(); entIt != vec.end(); entIt++)
	{
		if((*entIt)->id == x)
			return (*entIt);
	}

	for (entIt=vec.begin(); entIt != vec.end(); entIt++)
	{
		if(stricmp((*entIt)->name, text) == 0)
			return (*entIt);
	}

	return 0;
}



// Find an object within globals using unsigned id.
Entity* find_object(unsigned eid, VEC_ENT& vec)
{
	ENT_IT entIt;
	unsigned x = eid;

	if (eid == 0)
		return 0;

	for (entIt=vec.begin(); entIt != vec.end(); entIt++)
	{
		if((*entIt)->id == x)
			return (*entIt);
	}

	return 0;

}


// Restores entire database.
// FIXME: This should be broken down into different DB functions, for better compatability.
int readgame()
{
	Entity* tmp;
	VEC_ENT::size_type linksize, i;
	unsigned int writesize;
	unsigned int lid;

	ifstream ent_in(ENTITY_FILENAME, ios::in | ios::binary);
	if (ent_in.fail())
	{
		ent_in.close();
		return 0;
	}

	tmp = new Entity();
	tmp->Read(ent_in);

	while (!ent_in.eof())
	{
		if (tmp->type != player)
		{
			Globalize(tmp);
			tmp = new Entity();
		}
		else
		{
			reserveid(tmp->id);
			tmp->ResetData();
		}

		tmp->Read(ent_in);
	}

	delete tmp;		// last entity read is always bad

	// All static entities are loaded, re-read game data for linkage
	ent_in.clear();
	ent_in.seekg(0,ios::beg);

	// NOTE: the following data size must be changed if Entity::Write is altered
	writesize = ((sizeof(unsigned int)*2) + sizeof(DWORD) + MAX_ENTITY_NAME+1 +
				MAX_ENTITY_DESC+1 + sizeof(classtype) + (sizeof(int)*2));

	while (!ent_in.eof())
	{
		ent_in.read((char*)&lid, sizeof(unsigned int));
		if (ent_in.eof())
			break;

		tmp = find_object(lid, globals);

		// writesize takes filepointer from back of id to front of linksize
		ent_in.seekg(writesize, ios::cur);
		ent_in.read((char*)&linksize, sizeof(VEC_ENT::size_type));

		// non-areas will have 0 link ids stored
		if (!tmp || tmp->type != area)
			continue;

		for (i=0; i<linksize; i++)
		{
			ent_in.read((char*)&lid, sizeof(unsigned int));
			link(tmp, find_object(lid, globals));
		}
	}

	ent_in.close();

	return 1;

}



/* Sets setting of entref to specified flag (if valid). */
int set(Entity* ent, char* flag, char* setting)
{
	int err;

	if (!ent)
		return 0;

	/* FLAG TABLE */
	if (!stricmp(flag, "desc"))
		err = ent->SetDesc(setting);
	else if (!stricmp(flag, "name"))
		err = ent->SetName(setting);
	else if (!stricmp(flag, "type"))
		err = ent->SetType(setting);
	else if (!stricmp(flag, "hp"))
		err = ent->SetHP(setting);
	else if (!stricmp(flag, "irchost"))
	  err = ent->SetIrcHost(setting);
	else
		return 0;

	if (err == 0)
		ent->Send("Invalid flag argument(s).\n");


	return 1;
}


// Adds an entity to globals and reserves it's id. This needs to be done only once per
// game, as entity ids remain reserved on entity deletion unless explicitly removed.
void Globalize(Entity *ent)
{
	Entity* gent = (g_entIt !=  globals.end() ? (*g_entIt) : 0);

	reserveid(ent->id);
	globals.push_back(ent);
	g_entIt = findit(gent, globals);
}


void reserveid(unsigned id)
{
	int k = 0x1;
	int i = id%8;

	if (i)
		k = k << (i-1);
	else
		k = 0x0;

	id_list[(id/8)] |= k;

	return;
}



// lo mismo como reserveid
// never expects id of 0
void releaseid(unsigned id)
{
	int k = 0x1;
	int i = id%8;

	if (i)
		k = k << (i-1);
	else
		k = 0x0;

	id_list[(id/8)] &= ~k;

	return;
}


unsigned getnextid()
{
	int i, x, j;

	for (i=0; i<=65536;i++)
	{
		for (j=0, x=0x0;j<8;j++)
		{
			if (!(id_list[i] & x) && (i+j) != 0)	// not reserved and not 0
				return ( (8*i) + j);
			x = 0x1;
			x = x << j;
		}
	}

	return 0;
}



/*	takes empty (or not) dbl array of char and parses word for word into paramv
	returns index of paramv  (paramc). If paramv isn't clean, it WILL clean it for you.
*/
int parse_string (char** &paramv, const char* string)
{
	bool inquote = false;
	int paramc = 1;
	int i=0, j=0, k=0;
	int slen = strlen(string)+1;
	int wlen = 0;

	if (paramv)	// paramv is dynamic, so trash any pre-made data
		delete paramv;
	paramv = 0;



	// Get amount of parameters
	for(i=0;i<slen && string[i]!=0;i++)
	{
		if (string[i] == ' ' && string[i+1] == '\"') {
			paramc++;
			inquote = true;
			i++;	// skip past quote
		}
		else if (string[i] == '\"' && inquote)
			inquote = false;
		else if (string[i] == ' ' && string[i+1] != ' ' && !inquote)
			paramc++;
	}
	paramv = new char*[paramc];
	inquote = false;

	// Allocate appropriate space
	// Parse buffer into paramv
	for(i=0,j=0,wlen=0;i<slen && j<paramc;i++)
	{
		if (string[i] == '\"' && !inquote)
			inquote = true;
		else if ((string[i] == ' ' && string[i+1] != ' ' && !inquote) || string[i] == 0 ||
			(string[i] == '\"' && inquote))
		{
			paramv[j] = new char[wlen+1];
			for(k=0;k<wlen && string[(i-wlen)+k]!=0;k++)
				paramv[j][k]=string[(i-wlen)+k];

			paramv[j][wlen] = 0;
			j++;
			wlen=0;


		}
		else
			wlen++;
	}


	return paramc;
}



int link (Entity* a, Entity* b)
{
	ENT_IT entIt;

	if (!(a && b) || !(a->type == area && b->type == area))
		return 0;

	// clean out prior linkage of same two areas
	while ( (entIt = findit(b, a->links)) != a->links.end())
		a->links.erase(entIt);
	while ( (entIt = findit(a, b->links)) != b->links.end())
		b->links.erase(entIt);

	a->links.push_back(b);
	b->links.push_back(a);
	
	return 1;
}


int unlink (Entity* a, Entity* b)
{
	ENT_IT it;

	if (!(a && b) || !(a->type == area && b->type == area))
		return 0;

	if ((it = findit(b, a->links)) != a->links.end())
		a->links.erase(it);
	if ((it = findit(a, b->links)) != b->links.end())
		b->links.erase(it);
	
	return 1;
}


// removes non-characters
void clean_input (char* string, int len)
{
	int i;

	for (i=0; string[i] != 0; i++)
	{
		if (string[i] < 32 || string[i] == 127)
		{
			if (string[i] == '\b')	// backslash
			{
				if (i > 0)	{
					strcpy((char*)&string[i-1], (char*)&string[i+1]);
					i -= 2;
				}
				else	{
					strcpy((char*)&string[i], (char*)&string[i+1]);
					--i;
				}
			}
			
			else	{
				strcpy((char*)&string[i], (char*)&string[i+1]);
				--i;
			}
		}
	}
}


void trim_input(char* string, int len)
{
	int i;

	for (i=(len-1);i>=0;i--)
	{
		if ((int)string[i] == 13 || (int)string[i] == 10)
			string[i] = 0;
	}
}


bool eos(char* string)
{
	int len = strlen(string);
	if (string[len-1] == 13 || string[len-1] == 10 || string[len-1] == '\n')
		return true;
	else
		return false;
}


// Writes single entity to file.
// EVP
int Debug_DumpEntity (Entity* ent, char* filename)
{
	ofstream out;

	out.open(filename, ios::out | ios::binary);
	if (out.fail())
		return -1;

	ent->Write(out);

	out.close();
	return 1;
}

// Cycles through an entity vector. Used for debug-mode monitoring.
// Expects Valid Parameters.
void Debug_CycleVector (VEC_ENT& vec)
{
	int count;
	ENT_IT entIt;

	for (entIt=vec.begin(),count=0; entIt!=vec.end(); entIt++,count++)
		1;

	return;
}

