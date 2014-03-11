
//  Created by jl777
//  MIT License
//

#ifndef NXTAPI_NXTsock_h
#define NXTAPI_NXTsock_h
#include <sys/socket.h>


struct peer_info { int64_t uploaded,downloaded __attribute__ ((packed)); char ipaddr[INET6_ADDRSTRLEN]; } __attribute__ ((packed));

struct server_request
{
	int32_t retsize,argsize,variant,timestamp __attribute__ ((packed));
    int32_t total_minutes,forged_minutes,srcgateway,destgateway,numinputs,isforging __attribute__ ((packed));
    char NXTaddr[MAX_NXTADDR_LEN];
    union
    {
        struct
        {
            int64_t unspent,withdrawal,sum,ltbd __attribute__ ((packed));
            char withdrawaddr[MAX_COINADDR_LEN],redeem_txid[MAX_NXTTXID_LEN];
            unsigned char input_vouts[MAX_RAWINPUTS];
            char input_txids[MAX_RAWINPUTS][MAX_COINTXID_LEN],rawtransaction[4096],signedtransaction[4096];
        };
        struct peer_info peers[MAX_ACTIVE_PEERS];
    };
};

struct server_response
{
    int32_t retsize,tbd1,tbd2,tbd3;
    int64_t nodeshares,current_nodecoins,nodecoins,nodecoins_sent;
};
struct server_request WINFO[NUM_GATEWAYS];

int wait_for_serverdata(int *sockp,char *buffer,int len)
{
	int total,rc,sock = *sockp;
#ifdef __APPLE__
	if ( 0 && setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT,(char *)&len,sizeof(len)) < 0 )
	{
		printf("setsockopt(SO_RCVLOWAT) failed\n");
		close(sock);
		*sockp = -1;
		return(-1);
	}
#endif
    //printf("wait for %d\n",len);
	total = 0;
	while ( total < len )
	{
		rc = (int)recv(sock,&buffer[total],len - total, 0);
		if ( rc <= 0 )
		{
			if ( rc < 0 )
				printf("recv() failed\n");
			//else printf("The server closed the connection\n");
			close(sock);
			*sockp = -1;
			return(-1);
		}
		total += rc;
	}
	return(total);
}

int issue_server_request(struct server_request *req,int srcgateway)
{
	static int sd;
	int rc,variant,retsize;
	char server[128],servport[16] = SERVER_PORTSTR;
	struct in6_addr serveraddr;
	struct addrinfo hints, *res=NULL;
    if ( req->destgateway < 0 || req->destgateway >= NUM_GATEWAYS | srcgateway < 0 || srcgateway >= NUM_GATEWAYS )
    {
        printf("illegal dest gateway.%d srcgateway.%d\n",req->destgateway,srcgateway);
        return(-1);
    }
    variant = req->destgateway*NUM_GATEWAYS + srcgateway;
	sprintf(servport,"%d",SERVER_PORT + variant);
    printf("src.%d -> dest.%d variant.%d port.%s NXT.(%s)\n",srcgateway,req->destgateway,variant,servport,req->NXTaddr);
	//if ( sds[variant] < 0 )
	{
		strcpy(server, Server_names[req->destgateway]);
		memset(&hints, 0x00, sizeof(hints));
		hints.ai_flags    = AI_NUMERICSERV;
		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		/********************************************************************/
		/* Check if we were provided the address of the server using        */
		/* inet_pton() to convert the text form of the address to binary    */
		/* form. If it is numeric then we want to prevent getaddrinfo()     */
		/* from doing any name resolution.                                  */
		/********************************************************************/
		rc = inet_pton(AF_INET, server, &serveraddr);
		if ( rc == 1 )    // valid IPv4 text address? 
		{
			hints.ai_family = AF_INET;
			hints.ai_flags |= AI_NUMERICHOST;
		}
		else
		{
			rc = inet_pton(AF_INET6, server, &serveraddr);
			if ( rc == 1 ) // valid IPv6 text address? 
			{
				hints.ai_family = AF_INET6;
				hints.ai_flags |= AI_NUMERICHOST;
			}
		}
		/********************************************************************/
		/* Get the address information for the server using getaddrinfo().  */
		/********************************************************************/
		rc = getaddrinfo(server, servport, &hints, &res);
		if ( rc != 0 )
		{
			printf("Host not found --> %s\n", gai_strerror((int)rc));
			if (rc == EAI_SYSTEM)
				printf("getaddrinfo() failed\n");
			sd = -1;
			sleep(3);
			return(-1);
		}
        //printf("got serverinfo %s %s\n",server,servport);
		/********************************************************************/
		/* The socket() function returns a socket descriptor representing   */
		/* an endpoint.  The statement also identifies the address family,  */
		/* socket type, and protocol using the information returned from    */
		/* getaddrinfo().                                                   */
		/********************************************************************/
		sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sd < 0)
		{
			printf("socket() failed\n");
			sd = -1;
			sleep(3);
			return(-1);
		}
        //printf("socket created\n");
		/********************************************************************/
		/* Use the connect() function to establish a connection to the      */
		/* server.                                                          */
		/********************************************************************/
        //printf("try to connect to %s\n",Server_names[req->destgateway]);
		rc = connect(sd, res->ai_addr, res->ai_addrlen);
		if (rc < 0)
		{
			/*****************************************************************/
			/* Note: the res is a linked list of addresses found for server. */
			/* If the connect() fails to the first one, subsequent addresses */
			/* (if any) in the list could be tried if desired.               */
			/*****************************************************************/
			perror("connect() failed");
			printf("connection variant.%d srcgateway.%d -> destgateway.%d failure\n",variant,srcgateway,req->destgateway);
			close(sd);
			sd = -1;
			sleep(3);
			return(-1);
		}
		//printf("connected to variant.%d server.%d <- srcgateway.%d\n",variant,req->destgateway,srcgateway);
		if ( res != NULL )
			freeaddrinfo(res);
	}
    req->argsize = sizeof(*req);
    printf("send %d req %d bytes from gateway.%d -> dest.%d\n",sd,req->argsize,srcgateway,req->destgateway);
	if ( (rc = (int)send(sd,req,req->argsize,0)) < 0 )
	{
		printf("send(%d) request failed\n",variant);
		close(sd);
		sd = -1;
		sleep(1);
		return(-1);
	}
	//usleep(1);
    if ( variant == 0 )
    {
        retsize = sizeof(struct server_response);
        if ( retsize > 0 && (rc= wait_for_serverdata(&sd,(char *)req,retsize)) != retsize )
        {
            printf("GATEWAY_RETSIZE error\n");
            return(-1);
        }
    }
    close(sd);
    sd = -1;
	return(rc);
}

//#ifndef __APPLE__
int wait_for_client(char str[INET6_ADDRSTRLEN],int variant)
{
	static int sds[200];
	struct sockaddr_in6 serveraddr, clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	int i,sdconn = -1;
    str[0] = 0;
	if ( sds[0] == 0 )
	{
		for (i=0; i<200; i++)
			sds[i] = -1;
	}
	if ( variant < 0 )//|| (variant > 36 && variant != NUM_COMBINED) )
		perror("wait_for_client: variant < 0 || variant > 36");
	//get_lockid(0);
	while ( sds[variant] < 0 )
	{
		/********************************************************************/
		/* The socket() function returns a socket descriptor representing   */
		/* an endpoint.  Get a socket for address family AF_INET6 to        */
		/* prepare to accept incoming connections on.                       */
		/********************************************************************/
		if ((sds[variant] = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		{
			perror("socket() failed");
			break;
		}
		/********************************************************************/
		/* The setsockopt() function is used to allow the local address to  */
		/* be reused when the server is restarted before the required wait  */
		/* time expires.                                                    */
		/********************************************************************/
		if ( 0 )//setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,(char *)&on,sizeof(on)) < 0)
		{
			perror("setsockopt(SO_REUSEADDR) failed");
			close(sds[variant]);
			sds[variant] = -1;
			break;
		}
		/********************************************************************/
		/* After the socket descriptor is created, a bind() function gets a */
		/* unique name for the socket.  In this example, the user sets the  */
		/* address to in6addr_any, which (by default) allows connections to */
		/* be established from any IPv4 or IPv6 client that specifies port  */
		/* 3005. (i.e. the bind is done to both the IPv4 and IPv6 TCP/IP    */
		/* stacks).  This behavior can be modified using the IPPROTO_IPV6   */
		/* level socket option IPV6_V6ONLY if desired.                      */
		/********************************************************************/
		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin6_family = AF_INET6;
		serveraddr.sin6_port   = htons(SERVER_PORT+variant);
		/********************************************************************/
		/* Note: applications use in6addr_any similarly to the way they use */
		/* INADDR_ANY in IPv4.  A symbolic constant IN6ADDR_ANY_INIT also   */
		/* exists but can only be used to initialize an in6_addr structure  */
		/* at declaration time (not during an assignment).                  */
		/********************************************************************/
		serveraddr.sin6_addr   = in6addr_any;
		/********************************************************************/
		/* Note: the remaining fields in the sockaddr_in6 are currently not */
		/* supported and should be set to 0 to ensure upward compatibility. */
		/********************************************************************/
		//printf("start bind.variant.%d\n",variant);
		if ( bind(sds[variant],(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0 )
		{
			//fprintf(stderr,"%s sd.%d",jdatetime_str(actual_gmt_jdatetime()),sds[variant]);
			printf("variant.%d\n",variant);
			perror("variant bind() failed");
			close(sds[variant]);
			sds[variant] = -1;
			sleep(30);
			continue;
		}
		//printf("finished bind.variant.%d\n",variant);
		/********************************************************************/
		/* The listen() function allows the server to accept incoming       */
		/* client connections.  In this example, the backlog is set to 3  */
		/* This means that the system will queue 3 incoming connection     */
		/* requests before the system starts rejecting the incoming         */
		/* requests.                                                        */
		/********************************************************************/
		if (listen(sds[variant], 300) < 0)
		{
			perror("listen() failed");
			close(sds[variant]);
			sds[variant] = -1;
			break;
		}
		//printf("Ready for client connect(). ");
	}
	//release_lockid(0);
	
	if ( sds[variant] < 0 )
		return(-1);
	else
	{
		/********************************************************************/
		/* The server uses the accept() function to accept an incoming      */
		/* connection request.  The accept() call will block indefinitely   */
		/* waiting for the incoming connection to arrive from an IPv4 or    */
		/* IPv6 client.                                                     */
		/********************************************************************/
		if ((sdconn = accept(sds[variant], NULL, NULL)) < 0)	// non blocking would be nice
		{
			perror("accept() failed");
			return(-1);
		}
		else
		{
			/*****************************************************************/
			/* Display the client address.  Note that if the client is       */
			/* an IPv4 client, the address will be shown as an IPv4 Mapped   */
			/* IPv6 address.                                                 */
			/*****************************************************************/
			getpeername(sdconn, (struct sockaddr *)&clientaddr,&addrlen);
			if ( inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, INET6_ADDRSTRLEN) != 0 )
			{
				static unsigned long debugmsg;
				if ( debugmsg++ < 10 )
                {
                    int srcgateway,destgateway;
                    srcgateway = (variant % NUM_GATEWAYS);
                    destgateway = (variant / NUM_GATEWAYS);
                    printf("variant.%d src.%d -> dest.%d [Client address is %20s | Client port is %6d] sdconn.%d\n",variant,srcgateway,destgateway,str,ntohs(clientaddr.sin6_port),sdconn);
                }
			}
		}
	}
	return(sdconn);
}

void *_server_loop(void *_args)
{
    //static long total_minutes;
	static struct server_request *_REQ[200];
	struct server_request *req;
	int variant,sdconn,expected,rc,srcgateway,destgateway,bytesReceived,numreqs = 0;
	long xferred = 0;
    char clientip[INET6_ADDRSTRLEN];
	variant = *(int *)_args;
	if ( (req = _REQ[variant]) == 0 )
        _REQ[variant] = req = malloc(sizeof(*_REQ[variant]));
    srcgateway = (variant % NUM_GATEWAYS);
    destgateway = (variant / NUM_GATEWAYS);
	printf("Start server_loop.%d srcgateway.%d -> destgateway.%d on port.%d\n",variant,srcgateway,destgateway,SERVER_PORT+variant);
	while ( 1 )
	{
		usleep(100000);
		if ( (sdconn= wait_for_client(clientip,variant)) >= 0 )
		{
			expected = (int)sizeof(*req);// - sizeof(req->space));
			//printf("wait for req %d bytes from gateway.%d\n",expected,srcgateway);
			while ( 1 )
			{
				bytesReceived = 0;
				while ( bytesReceived < expected )
				{
					rc = (int)recv(sdconn,&((char *)req)[bytesReceived],expected - bytesReceived, 0);
					if ( rc <= 0 )
					{
						if ( rc < 0 )
							printf("recv() failed\n");
						else printf("The client closed the connection\n");
						break;
					}
					bytesReceived += rc;
					if ( rc >= 2 )
					{
						//printf("expected %d -> %d\n",expected,req->argsize);
						expected = req->argsize;
					}
                    //printf("gateway.%d got %d, total %d from gateway.%d\n",destgateway,rc,bytesReceived,srcgateway);
				}
				if ( bytesReceived < expected )
				{
					printf("The client.%d closed the connection before all of the data was sent, got %d of %d\n",variant,bytesReceived,expected);
                   // close(sdconn);
                   // sdconn = -1;
					break;
				}
                if ( variant == 0 )
                {
                    req->retsize = sizeof(struct server_response);
                    void process_nodecoin_packet(struct server_request *req,char *clientip);
                    process_nodecoin_packet(req,clientip);
                }
                else
                {
                    WINFO[srcgateway] = *req;
                    req->retsize = 0;
                }
				//int i;
				//for (i=0; i<bytesReceived; i++)
				//	printf("%02x ",((unsigned char *)req)[i]);
				//printf("| %d\n",rc);
 				//process_server_request(variant,req,req->space);
                if ( req->retsize > 0 )
                {
                    printf("return %d\n",req->retsize);
                    if ( (rc = (int)send(sdconn,req,req->retsize,0)) < req->retsize )
                    {
                        printf("send() failed? rc.%d instead of %d\n",rc,req->retsize);
                        break;
                    }
                    xferred += rc;
                }
				numreqs++;
                break;
			}
			close(sdconn);
			sdconn = -1;
		}
		//printf("Server.%d loop xferred %ld bytes, in %d REQ's ave %ld bytes\n",variant,xferred,numreqs,xferred/numreqs);
	}
}


#endif
