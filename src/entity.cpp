/*
	Basic Definition of In-Game Entites
	All Objects Derive from this Base Class
	- Roy Laurie

*/

#include "shared.h"
#include "entity.h"
#include "network.h"
#include "utility.h"
#include "account.h"
#include "globals.h"
#include "database.h"


// Return whether given entity is available.
// This is dependent on several factors such as classtype, connection, health, etc.
int Entity::Available ()
{
	if (type == player)
	{
		// Isn't connected
		if (sock == -1)
			return 0;
	}

	else if (type == item && owner)
	{
		// Owner (client) isn't connected
		if (owner && !owner->Available())
			return 0;
	}

	return 1;
}




void Entity::ResetData()
{
	int i=0;

	id = 0;
	memset(&name,0,MAX_ENTITY_NAME+1);
	memset(&desc,0,MAX_ENTITY_DESC+1);
	memset(&client_buffer, 0, MAX_BUFFER+1);
	flags = 0;
	isConnected = 0;
	pbuf = 0;
	hp = 0;
	max_hp = 0;
	weight = 0;
	max_weight = 0;
	owner = 0;
	location = 0;
	temp_flags=0;
	paramv = 0;
	paramc = 0;
	for(i=0;i<TINIT_MAX;i++)
		tInit[i] = 0;
	sock = -1;
	ip = 0;

	return;
}



Entity::Entity()
{
	ResetData();
	entity_count++;

	return;
}



Entity::Entity(char* aname, char* adesc, classtype aclass)
{
	char buffer[32];
	ResetData();

	memset(&buffer, 0, sizeof(buffer));
	strncpy(name, aname, MAX_ENTITY_NAME);
	strncpy(desc, adesc, MAX_ENTITY_DESC);

	id = getnextid();
	type = aclass;
	max_weight = (type != area ? 70 : 0);

	entity_count++;

	return;
}


/*	As you can see, destroying objects is a costly action.
		Try to keep it a minimum. =/
*/
Entity::~Entity()
{
	ENT_IT entIt, invIt;

	// Disconnect client
	if (sock != -1)
#ifdef BUILD_WINDOWS
			closesocket(sock);
#endif
#ifdef BUILD_LINUX
			close(sock);
#endif

	// Remove self from all entities containing self
	for (entIt=globals.begin(); entIt != globals.end(); entIt++)
	{
		// scan inventory
		for (invIt=(*entIt)->inventory.begin(); invIt != (*entIt)->inventory.end(); invIt++)
		{
			if((*invIt) == this)
			{
				invIt = (*entIt)->inventory.erase(invIt);
				invIt--;
			}
		}

		// scan links
		for (invIt=(*entIt)->links.begin(); invIt != (*entIt)->links.end(); invIt++)
		{
			if((*invIt) == this)
			{
				invIt = (*entIt)->links.erase(invIt);
				invIt--;
			}
		}
	}


	entity_count--;

	if (!(game_flags & GAMEFLG_SHUTDOWN || game_flags & GAMEFLG_INIT))
		printf ("Halted: %s(%d)\n", this->name, this->id);

	for (int i=0;i<paramc;i++)
		delete paramv[i];
}



int Entity::Think()
{
	int ret;

	this->temp_flags |= ET_THINKING;

	if (type == player)
		ret = ClientThink();

	else if (type == npc)
		ret = NpcThink();
	
	else if (type == area)
		ret = AreaThink();

	this->temp_flags &= ~ET_THINKING;

	return ret;
}



/*	For client/connected entities. Listens for and recieves command line from remote
	user.
*/
int Entity::ClientThink()
{
	int ret = 1, err, i=0;

	if (sock == -1)
		return 1;

	if (!FD_ISSET(sock, &readfs))
		return 1;

	// Get input
	pbuf = client_buffer + strlen(client_buffer);
	err = recv(sock, pbuf, MAX_BUFFER-strlen(client_buffer), 0);
	if (err == -1)
		return GAME_CLIENT_DISCONNECT;

	if (!eos(client_buffer))
		return 1;

	// Clean up input and parse
	trim_input(client_buffer, strlen(client_buffer));
	clean_input(client_buffer, strlen(client_buffer));
	paramc = parse_string(paramv, client_buffer);


	/* MESSAGE LOOP */

	if (!stricmp(paramv[0], "#cycle"))
		err = OnCycle();

	else if (!stricmp(paramv[0], "#dump"))
		err = OnDump();

	else if (stricmp(paramv[0], "@shutdown") == 0)
	{
		OnShutdown();
		return GAME_SHUTDOWN;
	}

	else if (stricmp(paramv[0], "@create") == 0)
		err = OnCreate();

	else if (stricmp(paramv[0], "@destroy") == 0)
		err = OnDestroy();

	else if (stricmp(paramv[0], "@size") == 0)
		err = Send("Size: %d\n", globals.size());

	else if (stricmp(paramv[0], "@set") == 0)
		err = OnSet();

	else if (!stricmp(paramv[0], "@link"))
		err = OnLink();

	else if (!stricmp(paramv[0], "@ircclients"))
		err = OnIrcClients();
		
	// requires return to notify main loop
	else if (stricmp(paramv[0], "@save") == 0)
		return OnSave();

	else if (stricmp(paramv[0], "say") == 0)
		err = OnSay();

	else if (stricmp(paramv[0], "help") == 0)
		err = OnHelp();
	
	else if (stricmp(paramv[0], "go") == 0)
		err = OnGo();

	else if (stricmp(paramv[0], "quit") == 0)
	{
		OnQuit();
		return GAME_CLIENT_DISCONNECT;
	}

	else if (stricmp(paramv[0], "look") == 0)
		err = OnLook();

	else
		err = Send("Huh? (try \'help\')\n");

	/* END MESSAGE LOOP */

	memset(&client_buffer, 0, sizeof(client_buffer));

	for (i=0;i<paramc;i++)
		delete paramv[i];

	paramv = 0;
	paramc = 0;

	if (err == -1)
		return GAME_CLIENT_DISCONNECT;

	if ( SendPrompt() == -1)
		return GAME_CLIENT_DISCONNECT;

	return ret;
}


// NOTE: Don't forget to update readgame() if data size serialized is altered!
ofstream& Entity::Write(ofstream& out)
{
	ENT_IT entIt;
	VEC_ENT::size_type linksize;
	unsigned wa = 0;

	out.write((char*) &id, sizeof(unsigned));
	out.write((char*) &flags, sizeof(DWORD));
	out.write(name, MAX_ENTITY_NAME+1);
	out.write(desc, MAX_ENTITY_DESC+1);
	out.write((char*) &(owner?owner->id:wa), sizeof(unsigned));
	out.write((char*) &(location?location->id:wa), sizeof(unsigned));
	out.write((char*) &type, sizeof(classtype));
	out.write((char*) &hp, sizeof(int));
	out.write((char*) &max_hp, sizeof(int));

	// Linkage
	linksize = links.size();
	out.write((char*) &linksize, sizeof(VEC_ENT::size_type));

	for (entIt=links.begin(); entIt!=links.end(); entIt++)
		out.write((char*) &(*entIt)->id, sizeof(unsigned int));
	

	temp_flags |= ET_WROTE;

	return out;
}



ifstream& Entity::Read(ifstream& in)
{
	VEC_ENT::size_type linksize;

	in.read((char*) &id, sizeof(unsigned));
	in.read((char*) &flags, sizeof(DWORD));
	in.read(name, MAX_ENTITY_NAME+1);
	in.read(desc, MAX_ENTITY_DESC+1);
	in.read((char*) &tInit[TINIT_OWNER], sizeof(unsigned));
	in.read((char*) &tInit[TINIT_LOCATION], sizeof(unsigned));
	in.read((char*) &type, sizeof(classtype));
	in.read((char*) &hp, sizeof(int));
	in.read((char*) &max_hp, sizeof(int));
	in.read((char*) &linksize, sizeof(VEC_ENT::size_type));
	
	// don't need to read these yet...
	in.seekg(sizeof(unsigned int) * linksize, ios::cur);

	temp_flags |= ET_READ;

	return in;
}



/*	Finalizes any initialization from such methods as media restore.
		NOTE: Call only AFTER globals is done reading!
*/
int Entity::InitData()
{
	Entity* ent = this;

	if (temp_flags & ET_READ)
	{
		owner = find_object(tInit[TINIT_OWNER], globals);
		location = find_object(tInit[TINIT_LOCATION], globals);
	}

	temp_flags |= ET_THINKING;

	// GHOST CHECK
	if (type == item && tInit[TINIT_OWNER] && !owner)	// item owner isn't alive/connected
		return 0;	// return this as unlinked
	
	if (type == player && !isConnected)
		return 0;
	//	END GHOST CHECK

	if (isConnected && type == player)
	{
		login_welcome(this);
		motd(this);
	}

	if (!location && type != area)
		Go(find_object(ID_HOME, globals));	// already here, so don't move there again

	else if (owner && type == item)
		Go(owner);
	
	else if (location && type != area)
		Go(location);

	if (type == player)
		SendPrompt();

	temp_flags &= ~ET_READ;
	temp_flags &= ~ET_THINKING;


	return 1;

}


// Looks at an entity. target is expected to be existant.
int Entity::Look (Entity *target)
{
	ENT_IT entIt;

	if (target->type == player || target->type == npc)
	{
		if ( Send ("%s (%d)\n%s\nHP:%d\n", target->name, target->id, target->desc, target->hp) == -1 )
			return -1;
	}
	else if (target->type == item)
	{
		if ( Send ("%s (%d)\n%s\n", target->name, target->id, target->desc) == -1 )
			return -1;
	}
	else
	{
		if ( Send("%s (%d)\n%s\n", target->name, target->id, target->desc) == -1 )
			return -1;

		for (entIt=target->inventory.begin(); entIt != target->inventory.end(); entIt++)
		{
			if ( Send("  %s (%d)\n", (*entIt)->name, (*entIt)->id) == -1 )
				return -1;
		}

		if (target->type == area)
		{
			if ( Send ("\nExits:\n") == -1 )
				return -1;

			for (entIt=target->links.begin(); entIt != target->links.end(); entIt++)
			{
				if ( Send("   %s\n", (*entIt)->name) == -1 )
					return -1;
			}
			if ( Send ("\n") == -1 )
				return -1;
		}	
	}

	return 1;
}



int Entity::Go(Entity* targ)
{
	Entity* ent = this;

	if (!targ || targ->type != area)
		return 0;

	if (location && !(ent->temp_flags & ET_READ))
	{
		location->Broadcast(location->inventory, "%s leaves towards the %s.\n", name, targ->name);
		location->inventory.erase(findit(ent, location->inventory));	// Leave old area
	}
	
	location = targ;
	location->inventory.push_back(this);		// Enter new area
	location->Broadcast(location->inventory, "%s arrives.\n", name);

	Look(targ);

	return 1;
}



int Entity::SetDesc (const char* newdesc)
{
	memset(this->desc,0,MAX_ENTITY_DESC+1);
	strncpy(this->desc,newdesc,MAX_ENTITY_DESC+1);
	return 1;
}



int Entity::SetName (const char* newname)
{
	memset(this->name, 0, MAX_ENTITY_NAME+1);
	strncpy(this->name, newname, MAX_ENTITY_NAME+1);
	return 1;
}



int Entity::SetType(const char* newtype)
{
	Entity* ent = this;

	if (!stricmp(newtype, "area"))
	{	
		if (location)
			location->inventory.erase(findit(ent, location->inventory));
		if (owner)
			owner->inventory.erase(findit(ent, owner->inventory));
		location = 0;
		owner = 0;
		weight = 0;
		type = area;
	}
	else if (!stricmp(newtype, "item"))
	{
		type = item;
		if (!location)
			Go(globals[ID_HOME]);
	}
	else if (!stricmp(newtype, "player"))
	{
		type = player;
		if (!location)
			Go(globals[ID_HOME]);
	}
	else if (!stricmp(newtype, "npc"))
	{
		type = npc;
		if (!location)
			Go(globals[ID_HOME]);
	}
	else
		return 0;

	return 1;

}

int Entity::SetHP (const char* flag)
{
	if (!flag)
		return 0;

	this->hp = this->max_hp = atoi(flag);

	return 1;
}

int Entity::SetIrcHost(const char* flag)
{
  if (!stricmp(flag, "true"))
    flags |= EFL_IRC_HOST;
  else if (!stricmp(flag, "false"))
    flags &= !EFL_IRC_HOST;
  else
    return 0;
    
  return 1;
}

int Entity::SendPrompt()
{
	if ( Send("[%d] > ", hp) == -1)
		return -1;

	return 1;
}




// Broadcasts to entity's inventory
// Used for areas mainly.
int Entity::Broadcast (VEC_ENT& vec, char* buffer, ...)
{
	ENT_IT entIt;
	char new_buffer[MAX_BUFFER+1];
	char conversion[64+1];
	int len = strlen(buffer);
	int err, i, j;
	va_list	arguments;

	va_start(arguments, buffer);
	memset(&new_buffer, 0, MAX_BUFFER+1);

	// Parse string
	for (i=0, j=0; i<len && j<MAX_BUFFER; i++, j++)
	{
		if (buffer[i] == '%')	// special character
		{
			memset(&conversion, 0, sizeof(conversion));

			switch (buffer[i+1])
			{
			case 's':			// string
				strncat(new_buffer, va_arg(arguments, char*), (MAX_BUFFER-1-strlen(new_buffer)));
				j = strlen(new_buffer)-1;
				break;

			case 'c':			// character
				new_buffer[j] = va_arg(arguments, char);
				break;

			case 'd':			// integer
				sprintf(conversion, "%d", va_arg(arguments, int));
				strncat(new_buffer, conversion, (MAX_BUFFER-1-strlen(new_buffer)));
				j = strlen(new_buffer)-1;
				break;

			case '%':			// percentage sign
			default:
				new_buffer[j] = '%';
			}

			i++; // next iteration will skip past character code set
		}
		else if (buffer[i] == '\n')
		{
			new_buffer[j] = '\n';
			new_buffer[j+1] = '\r';
			j++;
		}
		else
			new_buffer[j] = buffer[i];
	}

	va_end(arguments);
	len = strlen(new_buffer);

	for (entIt=vec.begin(); entIt != vec.end(); entIt++)
	{
		if ((*entIt)->type == player && (*entIt)->isConnected)
		{
			if (send ((*entIt)->sock, new_buffer, len, 0) == -1)
			{
				entIt = (*entIt)->Disconnect();
				entIt--;
				err = -1;
			}

			if (!((*entIt)->temp_flags & ET_THINKING))
			{
				if ( (*entIt)->SendPrompt() == -1)
				{
					entIt = (*entIt)->Disconnect();
					entIt--;
					err = -1;
				}
			}
		}

	}

	
	return 1;
}


ENT_IT Entity::Disconnect()
{
	Entity* ent = this;
	ENT_IT entIt;

	this->~Entity();
	sock = -1;
	entIt = globals.erase(findit(ent, globals));
	ghosts.push_back(this);

	cout << "Disconnection." << endl;

	return entIt;
}


//	Send data to client over network.
//	Allows usage like printf with special characters for data
//	substitution.
int Entity::Send (char* buffer, ...)
{
	char new_buffer[MAX_BUFFER+1];
	char conversion[64+1];
	int len = strlen(buffer);
	int err, i, j;
	va_list	arguments;

	if (this->type != player)
	 return 0;
 
	va_start(arguments, buffer);
	memset(&new_buffer, 0, MAX_BUFFER+1);

	// Parse string
	for (i=0, j=0; i<len && j<MAX_BUFFER; i++, j++)
	{
		if (buffer[i] == '%')	// special character
		{
			memset(&conversion, 0, sizeof(conversion));

			switch (buffer[i+1])
			{
			case 's':			// string
				strncat(new_buffer, va_arg(arguments, char*), (MAX_BUFFER-1-strlen(new_buffer)));
				j = strlen(new_buffer)-1;
				break;

			case 'c':			// character
				new_buffer[j] = va_arg(arguments, char);
				break;

			case 'd':			// integer
				itoa(va_arg(arguments, int), conversion, 10);
				strncat(new_buffer, conversion, (MAX_BUFFER-1-strlen(new_buffer)));
				j = strlen(new_buffer)-1;
				break;

			case '%':			// percentage sign
			default:
				new_buffer[j] = '%';
			}
			i++; // next iteration will skip past character code set
		}
		else if (buffer[i] == '\n')
		{
			new_buffer[j] = '\n';
			new_buffer[j+1] = '\r';
			j++;
		}
		else
			new_buffer[j] = buffer[i];
	}

	va_end(arguments);
	len = strlen(new_buffer);
	
	if (flags & EFL_IRC_CLIENT) {  // relays to irc_host + "TO:ID:..."
	  sprintf(conversion, "IRC:%d:", this->id); // using 'conversion' for this
	  err = irc_host->Send(conversion);
	  if (err != -1)
       err = irc_host->Send(new_buffer);
  } else    // normal telnet player
    err = send (sock, new_buffer, len, 0);
    
	if (err == -1) 
	 this->HandleBadSocket();
		return -1;
	
	return 1;
}

// strictly for debug purposes
// dumps global entity to a file specified by user
inline int Entity::OnDump()
{
	Entity* targ;
	char filename[MAX_PATH+1];
	int err, i, len;

	if (paramc <= 2)
		return Send("Format: #dump <entity> <filename>\n");

	targ = find_object(paramv[1], globals);
	if (!targ)
		return Send("Entity doesn't exist.\n");

	for (i=0, len=strlen(paramv[2]); i<len; i++)
	{
		if (paramv[2][i] == '\\' || paramv[2][i] == '/')
			return Send("Invalid filename.\n");
	}

	filename[0] = 0;
	strcpy(filename, "dump\\");
	strncat(filename, paramv[2], MAX_PATH - strlen(filename));

	if (Debug_DumpEntity(targ, filename) == -1)
		err = Send("Dump failed.\n");
	else
		err = Send("Dumped %s (%d) to %s\n", targ->name, targ->id, filename);

	return err;
}


inline int Entity::OnCycle()
{
	if (paramc <= 1)
		return Send("Format: #cycle <global vector>\n");

	if (!stricmp(paramv[1],"globals"))
		Debug_CycleVector (globals);
	else if (!stricmp(paramv[1],"ghosts"))
		Debug_CycleVector (ghosts);

	return 1;
}

inline int Entity::OnIrcClients()
{
  Entity* ent;
  
	if (paramc <= 2)
		return Send("Format: @ircclients <action> <parameters>\n");

	if (!stricmp(paramv[1],"login")) {
	  if (paramc <= 3)
	     return Send("Format: @ircclients login <username> <password>\n");
	     
		Login login(paramv[2], paramv[3]);
		if ((ent = login()))
		  return Send("IRC:%s:BAD_LOGIN", paramv[2]);
    else {
      inventory.insert(ent);
      return 1;
    }
 	}
 	
	else if (!stricmp(paramv[1],"logout"))
	  if (paramc <= 2)
	     return Send("Format: @ircclients logout <id>\n");
    if (
    

	return 1;
}

inline int Entity::OnSet()
{
	Entity* ent;
	int err = 1;
	int i, j;
	char* pbuffer = 0;

	if (paramc > 3)
	{
		for (i=0, j=0; i<MAX_BUFFER; i++)
		{
			if (j == 3)
			{
				pbuffer = client_buffer + i;	// set pbuffer to final argument of @set
				break;
			}

			if (client_buffer[i] == ' ')
				j++;
		}

		ent = find_object(paramv[1], globals);
		if (!ent)
		{
			err = Send("Unknown entity: %s.\n", paramv[1]);
			return err;
		}

		if(set(ent, paramv[2], pbuffer))
			err = Send ("Flag set.\n");
		else
			err = Send ("Unknown flag: %s.\n", paramv[2]);
	}
	else err = Send("Format: @set <entity> <flag> <setting>\n");

	return err;
}



inline int Entity::OnCreate()
{
	int err = 1;
	Entity* ent;

	if (paramc < 2)
	{
		err = Send("Format: create <type> <name>\n");
		return err;
	}

	if (!(!stricmp(paramv[1], "npc") || !stricmp(paramv[1], "player") ||
		!stricmp(paramv[1], "area") || !stricmp(paramv[1], "item")))
		return (Send("Invalid type\n"));

	ent = new Entity(paramv[2], "Entity");
	ent->SetType(paramv[1]);
	Globalize(ent);
	ent->InitData();
	if (ent->type != area)
		ent->Go(this->location);

	return err;
}



inline int Entity::OnGo()
{
	int err = 1;
	char* pname;

	if(paramc > 1 && location && !(flags & EFL_WARP))
	{
		pname = client_buffer + 3;	// "[g][o][ ][...]
		err = Go(find_object(paramv[1], location->links));
	}
	else if (paramc > 1 && location && (flags & EFL_WARP))
	{
		pname = client_buffer + 3;	// "[g][o][ ][...]
		err = Go(find_object(paramv[1], globals));
	}
	
	if (paramc < 1 || err == 0)
		err = Send("Format: go <id|name>\n");

	return err;
}



inline int Entity::OnDestroy()
{
	int err = 1;
	Entity* ent;
	ENT_IT entIt;

	if (paramc > 1)
	{
		ent = find_object(paramv[1], globals);
		if (ent)
		{
			ent->Broadcast(ent->inventory, "%s vanishes!\n", ent->name);

			// dump inventory to master room
			for (entIt=ent->inventory.begin(); entIt!=ent->inventory.end(); entIt++)
			{
				(*entIt)->Go(globals[INDX_HOME]);
				entIt--;
			}
			
			// unlink from all
			for (entIt=ent->links.begin(); entIt!=ent->links.end(); entIt++)
			{
				unlink(ent, (*entIt));
				entIt--;
			}

			globals.erase(findit(ent, globals));
			releaseid(ent->id);
			ent->temp_flags |= ET_DESTROY;
			delete ent;
		}
		else
			err = Send("Invalid entity\n");
	}
	else err = Send("Format: destroy <name|id>\n");

	return err;
}



inline int Entity::OnLook()
{
	int err = 1;
	Entity* targ;

	if (paramc > 1)
	{

		// look at entities first then exits if not found
		targ = find_object(paramv[1], location->inventory);
		if (!targ)
			targ = find_object(paramv[1], location->links);
		if (!targ)
			return ( Send("You don't see that here.\n") );

		err = Look(targ);
	}
	else
		err = Look(this->location);

	return err;
}



inline int Entity::OnSay()
{
	int err = 1;
	char* pbuf;

	if (paramc < 1)
		err = Send("Say what?\n");
	else
	{
		// locate message (non space delimited, can't use paramv)
		pbuf = client_buffer+4;
		location->Broadcast(location->inventory, "%s says, \"%s\"\n", this->name, pbuf);
	}

	return err;
}

inline int Entity::OnIrcClients()
{
  if (
  return err;
}

inline int Entity::OnLink()
{
	int err = 1;

	if (paramc > 2)
	{
	if (!link (find_object(paramv[1], globals), find_object(paramv[2], globals)))
		err = Send ("Erroneous linkage.\n");
	else
		err = Send ("Linked.\n");
	}
	else
		err = Send ("Format: @link <area1> <area2>\n");

	return err;
}


// client thinking process is expected to terminate it's host's existance
inline int Entity::OnQuit()
{
	int err = 1;

	err = Send("Goodbye.\n");

	return 1;
}



inline int Entity::OnSave()
{
	int err;
	// Save game-state
	Send("Saving...\n");
	Broadcast(globals, "This mists of time slow momentarily to a halt...\n");

	if (!DB_Entities_UpdateAll())	// removes all ghosts, no need to delete them later.
		err = GAME_SAVEFAIL;

	Broadcast(globals, "Existance echoes down the corridors of Time once again.\n");
	SendPrompt();


	return err;
}

static char* help_cmds[] =	{	"say",
								"go",
								"look",
								"quit",
								"@save",
								"@shutdown",
								"@set",
								"@create",
								"@destroy",
								"@link",
                "types"};

static char* help_params[] =	{	"<message>",
									"<entity>",
									"[entity]",
									"",
									"",
									"",
									"<entity> <flag> <value>",
									"<type> <name>",
									"<entity>",
									"<entity 1> <entity 2>",
                  ""};

static char* help_descs[] =	{	"Says a message outloud in the surrounding area.",
								"Changes entity's location to a given exit.",
								"Looks at a given entity. If no parameter is specified, looks at current area.",
								"Stores character and disconnects client from the game.",
								"Saves entire sever's game-state to disk.",
								"Saves game-state and completely stops all server activity.",
								"Changes attributes of a given entity.",
								"Creates a new entity and links it to the world. For a list of possible types, see\"types\".",
								"Removes an entity.",
								"Links an area to another area as an exit.",
                "NPC = Non-Player-Character (monsters, etc), Area=Location, Item=Something meant to be in an inventory."};



inline int Entity::OnHelp()
{
	int err = 1;
	int i, j;
	char* spacer = "     ";

	if (paramc > 1)
	{
		for(i=0; i<sizeof(help_cmds)/sizeof(help_cmds[0]);i++)
		{
			if (!stricmp(help_cmds[i], paramv[1]))
			{
				err = Send("%s %s\n%s\n", help_cmds[i], help_params[i], help_descs[i]);
				if (err == -1)
					return -1;
				else
					return 1;
			}

		}
	}

	for (i=0, j=0; i<sizeof(help_cmds)/sizeof(help_cmds[0]); i++)
	{
		if (j <= 45)
		{
			if (Send("%s    ", help_cmds[i]) == -1)
				return -1;
		}
		else
		{
			if (Send("\n%s    ", help_cmds[i]) == -1)
				return -1;
			j=0;
		}

		j += sizeof(help_cmds[i]) + 4;
	}

	err = Send("\n");
	return err;
}

void Entity::HandleBadSocket()
{
  // if it is an irc host or irc
    // search all entities for it's clients
      // disconnect each
  
  this->Disconnect();
}

inline int Entity::OnShutdown()
{
	return 1;
}







