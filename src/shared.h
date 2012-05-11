
#ifndef _SHARED_H_
#define _SHARED_H_

// BUILD PARAMETERS
	#define	BUILD_WINDOWS
//	#define	BUILD_LINUX
//	#define	BUILD_MACOS

// release version . compile version . databse version					
#define TBUD_VER		"0.9.4"

#define GAME_FAIL				-1
#define GAME_OK					0
#define GAME_SHUTDOWN			1001
#define GAME_SAVEOK				1002
#define GAME_SAVEFAIL			1003
#define GAME_LOAD				1004
#define GAME_CLIENT_DISCONNECT	1005


#define MAX_USER		8
#define MAX_PASS		8
#define MAX_BUFFER		255

// game_flags
#define GAMEFLG_INIT		0x1
#define GAMEFLG_SHUTDOWN	0x2

// Login stages
#define LSTG_PROMPT_OPTION		0
#define LSTG_RECV_OPTION		1
#define LSTG_PROMPT_USER		2
#define LSTG_RECV_USER			3
#define LSTG_PROMPT_PASS		4
#define LSTG_RECV_PASS			5
#define LSTG_LOGIN				6

// Database settings
#define LOGIN_FILENAME			"logins.dat"
#define ENTITY_FILENAME			"entities.dat"
#define LOGINWELCOME_FILENAME	"welcome.txt"
#define MOTD_FILENAME			"motd.txt"
#define MAX_GHOSTS	5

// Network settings
#define TB_BACKLOG	10
#define TB_PORT		3030




// ANSI Headers
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <math.h>
#include <errno.h>

// STL Headers
#include <vector>

#ifdef BUILD_WINDOWS
	#include <winsock.h>
#endif

#ifdef BUILD_LINUX
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <sys/types.h>
#endif

#include "tb_plat.h"


#define VEC_ENT		std::vector<Entity*>
#define VEC_PEND	std::vector<Pend*>
#define ENT_IT		std::vector<Entity*>::iterator
#define PEND_IT		std::vector<Pend*>::iterator


/*
Class Type Definitions:
	'player' - Remote client. human controlled and connection established.
	'npc' - Non-Player-Character. Emulates player, but controlled by server. No connection.
	'area' - Location. Base entity where all other entities reside in.
	'item' - Non-character. Used by other entities and rarely acts on it's own, if ever.
*/
enum classtype{player, npc, area, item};


#endif
