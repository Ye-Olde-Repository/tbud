
#ifndef _GLOBALS_H_
#define _GLOBALS_H_


// Global variables
extern ENT_IT g_entIt;
extern SOCKET listen_sock;
extern fd_set readfs, writefs;
extern int entity_count;
extern VEC_PEND pending;
extern VEC_ENT	globals;	
extern VEC_ENT	ghosts;
extern unsigned id_list[8192];
extern int game_flags;

#endif

