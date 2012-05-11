
#include "shared.h"
#include "entity.h"
#include "network.h"
#include "account.h"

// These are global to all aspects of the engine

ENT_IT g_entIt;

// Sockets
SOCKET listen_sock;
fd_set readfs, writefs;

int game_flags = 0;
int entity_count = 0;
unsigned id_list[(65536 / 8)];

VEC_ENT globals;	
VEC_ENT ghosts;
VEC_PEND pending;
