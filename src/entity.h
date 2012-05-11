
/*
	Basic Definition of In-Game Entites
	All Objects Derive from this Class
	- Roy Laurie

*/

#ifndef _ENTITY_H_
#define _ENTITY_H_

#define MAX_ENTITY_NAME 32
#define MAX_ENTITY_DESC 255

#define TINIT_MAX		5
#define TINIT_LOCATION	0
#define TINIT_OWNER		1

#define ID_HOME			1
#define INDX_HOME		0
#define ID_GOD			2
#define INDX_GOD		1

// Entity::flags
#define EFL_WARP		    0x00000002		// allows to move from global list, rather than local
#define EFL_IRC_HOST    0x00000004    // transparent entity that relays for several irc_client entities
#define EFL_IRC_CLIENT  0x00000008    // actual entity with output/input relayed from irc host

// Entity::temp_flags
#define ET_WROTE	0x00000001
#define ET_READ		0x00000002
#define ET_DESTROY	0x00000004
#define ET_THINKING	0x00000008

class Entity
{
public:
	Entity();
	Entity(char* aname, char* adesc, classtype aclass=npc);
	~Entity();
	void ResetData();
	int InitData();

	// Functions
	int	Look (Entity *target);			// retrieve charactersitics of an entity
	int	Go (Entity* targ);	// move entity to location	
	void Emote (char* text);			// display entity action
	int Available ();

	// Modifiers
	int	SetName (const char* newname);
	int	SetDesc (const char* newdesc);
	int	SetType (const char* newtype);
	int	SetHP (const char* flag);
	int SetIrcHost(const char* flag);

	// Characteristics
	int flags;						// entity flags
	unsigned int id;						// Unique ID for categorizing.
	char name[MAX_ENTITY_NAME+1];		// Name of Entity.
	char desc[MAX_ENTITY_DESC+1];		// Description of Entity.
	int hp;								// Current amount of Entity helath.
	int max_hp;							// Maximum amount of health allowable for entity.
	int weight;							// Amount of weight carried.
	int max_weight;						// Maximum weight allowed to carry.

	int Think();		// Client-frame operation handler
	int	ClientThink();	// Connection-frame procedure
	int NpcThink() { return 1;}		// Controlled entity procedure
	int AreaThink() { return 1;}

	ofstream& Write(ofstream& out);		// Save entity to media
	ifstream& Read(ifstream& in);		// Retrieve entity from media

	// Flags
	int temp_flags;
	classtype type;				// Defines of which class this entity belongs to: player, npc, area,
								// etc.

	Entity* owner;				// Pointer to any entity carrying this. (not for areas)
	Entity* location;			// Pointer to current area location (not for areas).
	Entity* irc_host;     // for clients using an irc_host as a proxy

	VEC_ENT inventory;	// References to current inventory. Includes clients, items, everything.
	VEC_ENT links;		// Links to other areas, aka exits (area only)


	//  Entity Response Functions (command-line response)
	int OnDump();
	int OnCycle();
	int OnHelp();
	int OnSay();
	int OnSet();
	int OnIrcClients();
	int OnCreate();
	int OnDestroy();
	int OnGo();
	int OnLook();
	int OnLink();
	int OnQuit();
	int OnSave();
	int OnShutdown();
	void HandleBadSocket(void);
	

	// Client-Based Networking
	SOCKET sock;
	int isConnected;	// 1=Yes 0=No -1=N/A (not player)
	unsigned long ip;

	// Network Functions
	int Send (char* buffer, ...);
	int Broadcast (VEC_ENT&, char* buffer, ...);
	ENT_IT Disconnect();
	int SendPrompt();

	// Client Thinking
	char client_buffer[MAX_BUFFER+1];
	
	// Database
	int Write(); // calls on database to write or append entity's data
	int Read(int offset);  // reads db data of offset into entity

protected:
	char* pbuf;
	char** paramv;	// input buffer
	int paramc;		// input buffer index
	int tInit[TINIT_MAX];	// For load/save usage.
};

#endif
