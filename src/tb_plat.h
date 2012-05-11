#ifndef _TB_PLAT_H_
#define _TB_PLAT_H_


#ifdef BUILD_WINDOWS

	#define errno WSAGetLastError()
	#define SOCKET unsigned int

#endif

#ifdef BUILD_LINUX
#define SOCKET unsigned int
inline int closesocket(SOCKET s) { return close(s); }
inline int stricmp(const char* s1, const char* s2) {return strcasecmp(s1, s2); }
typedef unsigned int DWORD;

#endif

#endif
