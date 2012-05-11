/*
	Text-Based User-Driven (TBUD) Game Engine

	1999 - Roy Laurie

*/

#include "shared.h"
#include "entity.h"
#include "utility.h"
#include "network.h"
#include "account.h"
#include "database.h"
#include "globals.h"

int RunTbud();

static char welcome_menu[] =	"\r\nPlease choose a number from the following menu:\r\n[1] Connect with account.\r\n[2] Create new account.\r\n> ";
								

int main (int argc, char **argv)
{
	if (!InitGame())
		return 0;

	if (!InitNetwork())
		return 0;

	RunTbud();

	return 0;
}
	

int RunTbud()
{
	int iop;
	char options[12];
	Entity *ent, *tmp_ent;
	Pend *pend;
	ENT_IT	entIt;
	PEND_IT	pendIt;
	bool rungame = 1;
	int gcmd = 0;
	int err;
	char *pstream;
	unsigned int gt_sock;
	struct timeval tv;



	// Core game loop
	while(rungame)
	{
		// check logins first
		gt_sock = 0;

		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		
		FD_ZERO(&readfs);
		FD_ZERO(&writefs);

		for (pendIt=pending.begin(); pendIt != pending.end(); pendIt++)
		{
			
			FD_SET((*pendIt)->sock, &readfs);
			FD_SET((*pendIt)->sock, &writefs);

			if (gt_sock < (*pendIt)->sock)
				gt_sock = (*pendIt)->sock;
		}


		err = select(gt_sock+1, &readfs, &writefs, NULL, &tv);
		if (err == -1 && pending.size() > 0)
		{
			cout << strerror(errno) << " (" << errno << ")\n";
			ShutdownGame();
		}

		for (pendIt=pending.begin(); pendIt != pending.end(); pendIt++)
		{
			pend = (*pendIt);
			memset(&options, 0, sizeof(options));

			if (FD_ISSET(pend->sock, &writefs) && pend->stage == LSTG_PROMPT_OPTION)
			{
				err = send((*pendIt)->sock, welcome_menu, sizeof(welcome_menu)-1, 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				pend->stage++;				
			}
			else if (FD_ISSET(pend->sock, &readfs) && pend->stage == LSTG_RECV_OPTION)
			{
				pstream = pend->input + strlen(pend->input);
				err = recv(pend->sock, pstream, (sizeof(pend->input)-2)-strlen(pend->input), 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				if (!eos(pend->input))
					continue;

				trim_input(pend->input, sizeof(pend->input));
				clean_input(pend->input, sizeof(pend->input));

				err = send(pend->sock, "\n\r", 2, 0);

				iop = atoi(pend->input);
				if (iop < 1 || iop > 2)
					pend->stage = LSTG_PROMPT_OPTION;
				else
				{
					if (iop == 2)
						pend->options |= PEND_CREATE;

					pend->stage++;
				}
			}
			if (FD_ISSET(pend->sock, &writefs) && pend->stage == LSTG_PROMPT_USER)
			{
				err = send(pend->sock, "User: ", 6, 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				pend->stage++;				
			}
			else if (FD_ISSET(pend->sock, &readfs) && pend->stage == LSTG_RECV_USER)
			{
				pstream = pend->login.user + strlen(pend->login.user);
				err = recv(pend->sock, pstream, MAX_USER-strlen(pend->login.user), 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				if (!eos(pend->login.user))
					continue;

				trim_input(pend->login.user, strlen(pend->login.user));
				clean_input(pend->login.user, strlen(pend->login.user));
				pend->stage++;
				
			}
			else if (FD_ISSET(pend->sock, &writefs) && pend->stage == LSTG_PROMPT_PASS)
			{
				err = send(pend->sock, "Password: ", 10, 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				pend->stage++;
			}
			else if (FD_ISSET(pend->sock, &readfs) && pend->stage == LSTG_RECV_PASS)
			{
				pstream = pend->login.password + strlen(pend->login.password);
				err = recv(pend->sock, pstream, MAX_PASS-strlen(pend->login.password), 0);
				if (err == -1)
				{
					delete (*pendIt);
					pendIt = pending.erase(pendIt);
					pendIt--;
					continue;
				}

				if (!eos(pend->login.password))
					continue;

				trim_input(pend->login.password, strlen(pend->login.password));
				clean_input(pend->login.password, strlen(pend->login.password));
				
				if (pend->options & PEND_CREATE)
				{
					// create login
					ent = new Entity(pend->login.user, "A new player!", player);
					Globalize(ent);
					DB_Logins_Write(pend->login);
				}
				else
				{
					// check login
					ent = DB_FindEntity(&pend->login);
					if (!ent)
					{
						err = send(pend->sock, "Login Incorrect.\n", 17, 0);
						delete (*pendIt);
						pendIt = pending.erase(pendIt);
						pendIt--;
						continue;
					}
					else
					{
						err = send(pend->sock, "\n\n", 2, 0);
						if (err == -1)
						{
							delete ent;
							delete (*pendIt);
							pendIt = pending.erase(pendIt);
							pendIt--;
							continue;
						}

						// make sure entity isn't cached
						tmp_ent = find_object(ent->id, ghosts);
						if (tmp_ent)
						{
							delete ent;
							ent = tmp_ent;
							ghosts.erase(findit(tmp_ent, globals));
						}

						tmp_ent = find_object(ent->id, globals);
						if (tmp_ent)
						{
							err = send(pend->sock, "Connection re-established.\n", 27, 0);
							if (err == -1)
							{
								delete ent;
								delete (*pendIt);
								pendIt = pending.erase(pendIt);
								pendIt--;
								continue;
							}

							delete ent;
							closesocket(tmp_ent->sock);
							ent = tmp_ent;
						}
						else
							globals.push_back(ent);
					}
				}

				// transfer connection to entity
				ent->sock = pend->sock;
				pend->sock = -1;
				ent->isConnected = 1;
				delete (*pendIt);
				pendIt = pending.erase(pendIt);
				pendIt--;
				if (ent->InitData() == -1)
				{
					delete ent;
					globals.erase(findit(ent, globals));
				}
					 

			}
		}

		// Work clients
		gt_sock = listen_sock;
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		
		FD_ZERO(&readfs);
		FD_ZERO(&writefs);
		FD_SET(listen_sock, &readfs);

		for (entIt=globals.begin(); entIt != globals.end(); entIt++)
		{
			ent = (*entIt);

			if (ent->isConnected != 1)
				continue;

			FD_SET(ent->sock, &readfs);
			FD_SET(ent->sock, &writefs);
			if (gt_sock < ent->sock)
				gt_sock = ent->sock;
		}

		err = select(gt_sock+1, &readfs, &writefs, NULL, &tv);
		if (err == -1)
		{
			cout << "error: could not select.\n";
			ShutdownGame();
		}

		if (FD_ISSET(listen_sock, &readfs))
			ScanLogin();

		for (g_entIt=globals.begin(); g_entIt != globals.end(); g_entIt++)
		{
			gcmd = (*g_entIt)->Think();

			if(gcmd == GAME_SHUTDOWN)
			{
				cout << "@shutdown called by " << (*g_entIt)->name << "." << endl;
				ShutdownGame();
			}
			else if (gcmd == GAME_SAVEOK || gcmd == GAME_SAVEFAIL)
			{
				cout << "@save called by " << (*g_entIt)->name << ".\nSaving Game...";
				if (gcmd == GAME_SAVEOK)
					cout << "done." << endl;
				else
					cout << "failed." << endl;

			}
			else if (gcmd == GAME_CLIENT_DISCONNECT)
			{
				g_entIt = (*g_entIt)->Disconnect();
				g_entIt--;
			}
			
			if (ghosts.size() > MAX_GHOSTS)
				DB_Entities_UpdateAll();

		}
	}

	
	return 1;
}


