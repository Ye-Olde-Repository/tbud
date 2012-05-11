
#ifndef _LOGIN_H_
#define _LOGIN_H_


//class ClientInfo
//{
//}

class Login
{
public:
	Login();
	Login(const char* u, const char* p);

	ofstream& Write(ofstream& out);
	ifstream& Read(ifstream& in);

	bool operator== (Login &rhs);
	bool operator() (void);

public:
	char user[MAX_USER+1];
	char password[MAX_PASS+1];
	unsigned id;
	SOCKET sock;
};



#define PEND_CREATE		0x00000001

// stages:
// 0-ask for user, 1-receive user, 2-ask for pass, 3-recv pass

class Pend
{
public:
	Pend();
	~Pend();

public:
	Login login;
	SOCKET sock;
	int stage;
	int options;
	char input[9];
};

#endif

