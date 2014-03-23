#ifndef gateway_multiplatform_h
#define gateway_multiplatform_h

int CloseSocket(int socket) {
#ifndef _WIN32
	// int close(int fd);
	return close(socket);
#else
	// int closesocket(_In_  SOCKET s);
	return closesocket(socket);
#endif
} 

void sleepOS(unsigned int seconds) {
#ifndef _WIN32
	// unsigned int sleep(unsigned int seconds)
	sleep(seconds);
#else
	// VOID WINAPI Sleep(DWORD dwMilliseconds)
	Sleep(seconds * 1000);
#endif
} 

void usleepOS(unsigned long usec) {
#ifndef _WIN32
	usleep(usec);
#else
	// 
http://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw
    __int64 time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *) &time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do {
        QueryPerformanceCounter((LARGE_INTEGER *) &time2);
    } while((time2-time1) < usec);
#endif
} 

#endif
