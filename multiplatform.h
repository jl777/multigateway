#ifndef gateway_multiplatform_h
#define gateway_multiplatform_h

#ifndef _WIN32
// UNIX
int OSinit() {
    // no action currently for non-Windows
    return(0);
}

int OScleanup() {
    // no action currently for non-Windows
    return(0);
}
#else 
// WINDOWS
/*
 * http://programmingrants.blogspot.com/2009/09/tips-on-undefined-reference-to.html
 * This fixes getaddrinfo not resolved error when linking.
 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>	

/* 
 * http://sourceforge.net/p/libmms/bugs/10/
 * AI_NUMERICSERV is not supported by mingw, or on pre-vista versions of windows
 * so the function will fail to work as expected when called. 
 * #define AI_NUMERICSERV 0x00000008  // Servicename must be a numeric port number
 * Probably a better fix with this is to make it an empty flag: AI_NUMERICSERV 0
 */
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif

/*
 * http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
 * EAI_SYSTEM: Other system error, check errno for details.
 */
#ifndef EAI_SYSTEM
#define EAI_SYSTEM -1   // value doesn't matter 
#endif

/* 
 * inet_pton and inet_ntop are not available in Windows pre-Vista environments like MinGW.
 * http://stackoverflow.com/questions/13731243/what-is-the-windows-xp-equivalent-of-inet-pton-or-inetpton
 * http://www.beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
 */
#include <string.h>
int inet_pton(int af, const char *src, void *dst)
{
  struct sockaddr_storage ss;
  int size = sizeof(ss);
  char src_copy[INET6_ADDRSTRLEN+1];

  ZeroMemory(&ss, sizeof(ss));
  /* stupid non-const API */
  strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
  src_copy[INET6_ADDRSTRLEN] = 0;

  if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
    switch(af) {
      case AF_INET:
    *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
    return 1;
      case AF_INET6:
    *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
    return 1;
    }
  }
  return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  struct sockaddr_storage ss;
  unsigned long s = size;

  ZeroMemory(&ss, sizeof(ss));
  ss.ss_family = af;

  switch(af) {
    case AF_INET:
      ((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
      break;
    case AF_INET6:
      ((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
      break;
    default:
      return NULL;
  }
  /* cannot directly use &size because of strict aliasing rules */
  return (WSAAddressToString((struct sockaddr *)&ss, sizeof(ss), NULL, dst, &s) == 0)?
          dst : NULL;
}

#define send sendOS
#define sleep sleepOS
#define usleep usleepOS

// UNIX:     ssize_t send(int sockfd, const void *buf, size_t len, int flags);
// WINDOWS:  int send(SOCKET s, const char *buf, int len, int flags);
// Typecasting for Windows send function
ssize_t sendOS(int sockfd, const void *buf, size_t len, int flags) {
    send((SOCKET) sockfd, (const char *) buf, (int) len, flags);    
}

void sleepOS(unsigned int seconds) {
    // VOID WINAPI Sleep(DWORD dwMilliseconds)
    Sleep(seconds * 1000);
} 

void usleepOS(unsigned long usec) {
    // http://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw
    __int64 time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *) &time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do {
        QueryPerformanceCounter((LARGE_INTEGER *) &time2);
    } while((time2-time1) < usec);
} 

/*
 * Initialize Winsock
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms737591%28v=vs.85%29.aspx
 */
int OSinit() {
	int iResult;
	WSADATA wsaData;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
	return iResult;
}

int OScleanup() {
	return WSACleanup();
}
#endif

int CloseSocket(int socket) {
#ifndef _WIN32
    // int close(int fd);
    return close(socket);
#else
    // int closesocket(_In_  SOCKET s);
    return closesocket(socket);
#endif
}

#endif // gateway_multiplatform_h
