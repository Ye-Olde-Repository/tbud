


#include "shared.h"
#include "account.h"
#include "database.h"
#include "utility.h"
#include "globals.h"






/* CLASS - LOGIN */
Login::Login()
{
	id = -1;
	memset(&user, 0, MAX_USER+1);
	memset(&password, 0, MAX_PASS+1);
	sock = -1;
}

Login::Login(const char* u, const char* p)
{
  int len;
  id = -1;
  sock = -1;
  
  len = strlen(u);
  //strncpy(user, u, (len<MAX_USER:len?MAX_USER));

  len = strlen(p);
  //strncpy(password, p, (len<MAX_PASS:len?MAX_PASS));
}


ofstream& Login::Write(ofstream& out)
{
	out.write(user, MAX_USER);
	out.write(password, MAX_PASS);
	out.write((char*)&id, sizeof(int));

	return out;
}


Entity* Login::operator() (void)
{
  Entity* ent;
  
  ent = DB_FindEntity(this);
  if (!ent)
    return 0;
  
  ent->sock = this->sock;
  ent->isConnected = 1;
	
  if (ent->InitData() == -1) {
    delete ent;
    globals.erase(findit(ent, globals));
    return 0;
  } 
  
  return ent;
}


ifstream& Login::Read(ifstream& in)
{
	in.read(user, MAX_USER);
	in.read(password, MAX_PASS);
	in.read((char*)&id, sizeof(int));

	return in;
}



bool Login::operator== (Login &rhs)
{
	if (!strcmp(this->user, rhs.user) && !strcmp(this->password, rhs.password))
		return true;
	else
		return false;
}

/* END CLASS LOGIN */



/* CLASS - PEND */
Pend::Pend()
{
	memset(&input, 0, sizeof(input));
	SOCKET sock;
	options = 0;
	stage = 0;
}


Pend::~Pend()
{
	if (sock != -1)
#ifdef BUILD_WINDOWS
		closesocket(sock);
#endif
#ifdef BUILD_LINUX
		close(sock);
#endif
}


