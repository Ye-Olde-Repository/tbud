

#include "shared.h"
#include "entity.h"
#include "network.h"
#include "account.h"
#include "globals.h"


static unsigned connect_waiting	= 0;
static struct sockaddr_in local_addr;

/* Call OS-Dependent Initiation for networking. */
int InitNetwork()
{
	int err = 0;

	// Setup local address
	local_addr.sin_addr.s_addr = INADDR_ANY;
	local_addr.sin_port = htons(TB_PORT);
	local_addr.sin_family = AF_INET;
	memset(&local_addr.sin_zero,0,8);


#ifdef BUILD_WINDOWS
	WORD wVersionRequested = MAKEWORD(2,2);
	WSADATA wsaData;

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err == -1)	{
		cout << "WSA Initiation Failed!\n";
		return 0;
	}
#endif /* End WINDOWS */

	listen_sock = socket (AF_INET, SOCK_STREAM, 0);

	err = bind (listen_sock, (struct sockaddr*)&local_addr, sizeof (struct sockaddr_in));
	if (err == -1)	{
		cout << "Unable to bind to listen socket.\n";
		return 0;
	}

	// Open server for connections with nonblocking socket
#ifdef BUILD_WINDOWS
	unsigned long ctl_cmd = 1;
	ioctlsocket(listen_sock, FIONBIO, &ctl_cmd);
#endif

#ifdef BUILD_LINUX
	fcntl (listen_sock, F_SETFL, O_NONBLOCK);
#endif

	listen(listen_sock, TB_BACKLOG);

	return 1;
}



/*	Scan local socket for new connections and deal with new/pending connections
	Returns -1 on error, 0 if nothing done, and 1 if connection(s) handled
*/
int ScanLogin ()
{
	Pend* pend;
	SOCKET newsock;
	struct sockaddr_in their_addr;
	int err = 0;
	int sin_size = sizeof(struct sockaddr);

	// Poll local socket for connections
	newsock = accept(listen_sock, (struct sockaddr*)&their_addr, &sin_size);
	if (newsock == -1 && errno != EAGAIN)
			return 0;
	
	pend = new Pend;
	pend->sock = newsock;
	pending.push_back(pend);

	cout << "Connection from " << inet_ntoa(their_addr.sin_addr) << endl;

	return 1;
}








