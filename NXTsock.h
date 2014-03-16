
//  Created by jl777
//  MIT License
//

#ifndef NXTAPI_NXTsock_h
#define NXTAPI_NXTsock_h
#include <sys/socket.h>

typedef int (*handler)();
struct handler_info { handler variant_handler; int32_t variant,funcid; long argsize,retsize; char **whitelist; };
static int Numhandlers;
static struct handler_info Handlers[100];
struct server_request WINFO[NUM_GATEWAYS];

int register_variant_handler(int variant,handler variant_handler,int funcid,long argsize,long retsize,char **whitelist)
{
    int i;
    if ( Numhandlers > (int)(sizeof(Handlers)/sizeof(*Handlers)) )
    {
        printf("Out of space: Numhandlers %d\n",Numhandlers);
        return(-1);
    }
    for (i=0; i<Numhandlers; i++)
    {
        if ( Handlers[i].variant == variant && Handlers[i].funcid == funcid )
        {
            printf("Overwriting handler for variant.%d funcid.%d\n",variant,funcid);
            Handlers[i].variant_handler = variant_handler;
            return(i);
        }
    }
    printf("Setting handler.%d for variant.%d funcid.%d\n",i,variant,funcid);
    Handlers[i].variant_handler = variant_handler;
    Handlers[i].funcid = funcid;
    Handlers[i].variant = variant;
    Handlers[i].argsize = argsize;
    Handlers[i].retsize = retsize;
    Handlers[i].whitelist = whitelist;
    return(Numhandlers++);
}

int find_handler(int variant,int funcid)
{
    int i;
    for (i=0; i<Numhandlers; i++)
    {
        if ( Handlers[i].variant == variant && Handlers[i].funcid == funcid )
            return(i);
    }
    return(-1);
}

int check_whitelist(char **whitelist,char *ipaddr)
{
    int i;
    if ( whitelist == 0 )
        return(0);
    for (i=0; whitelist[i]!=0; i++)
        if ( strcmp(whitelist[i],ipaddr) == 0 )
            return(i);
    printf("%s not in whitelist\n",ipaddr);
    return(-1);
}

int process_client_packet(int variant,struct server_request *req,char *clientip)
{
    int ind;
    char **whitelist;
    ind = find_handler(variant,req->H.funcid);
    if ( ind >= 0 )
    {
        whitelist = Handlers[ind].whitelist;
        if ( check_whitelist(whitelist,clientip) >= 0 )
            return((*Handlers[ind].variant_handler)((void *)req,clientip));
    }
    else
    {
        whitelist = Server_names;
        if ( check_whitelist(whitelist,clientip) >= 0 )
            WINFO[req->srcgateway % NUM_GATEWAYS] = *req;
    }
    req->H.retsize = 0;
    //printf("No handler for variant.%d funcid.%d (%s)\n",variant,req->funcid,clientip);
    return(0);
}

int wait_for_serverdata(int *sockp,unsigned char *buffer,int len)
{
	int total,rc,sock = *sockp;
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

int server_request(char *destserver,struct server_request_header *req,int32_t variant,int32_t funcid)
{
	int rc,retsize,ind,sd;
	char server[128],servport[16] = SERVER_PORTSTR;
	struct in6_addr serveraddr;
	struct addrinfo hints, *res=NULL;
    req->variant = variant;
    req->funcid = funcid;
    ind = find_handler(variant,funcid);
    //if ( ind < 0 )
    //    return(0);
 	sprintf(servport,"%d",SERVER_PORT + variant);
    strcpy(server, destserver);
    memset(&hints, 0x00, sizeof(hints));
    hints.ai_flags    = AI_NUMERICSERV;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
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
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0)
    {
        printf("socket() failed\n");
        sd = -1;
        sleep(3);
        return(-1);
    }
    rc = connect(sd, res->ai_addr, res->ai_addrlen);
    if ( rc < 0 )
    {
        perror("connect() failed");
        printf("connection variant.%d failure\n",variant);
        close(sd);
        sd = -1;
        //sleep(3);
        return(-1);
    }
    //printf("connected to variant.%d server.%d <- srcgateway.%d\n",variant,req->destgateway,srcgateway);
    if ( res != NULL )
        freeaddrinfo(res);
    if ( ind >= 0 && req->argsize == 0 )
        req->argsize = (int)Handlers[ind].argsize;
    if ( req->argsize == 0 )
        req->argsize = sizeof(struct server_request);
    //printf("send %d req %d bytes from variant.%d\n",sd,req->argsize,variant);
    if ( (rc = (int)send(sd,req,req->argsize,0)) < 0 )
    {
        printf("send(%d) request failed\n",variant);
        close(sd);
        sd = -1;
        //sleep(1);
        return(-1);
    }
    //usleep(1);
    retsize = req->retsize;
    if ( ind >= 0 && req->retsize == 0 )
        retsize = (int)Handlers[ind].retsize;
    else retsize = 0;
    if ( retsize > 0 && (rc= wait_for_serverdata(&sd,(unsigned char *)req,retsize)) != retsize )
    {
        printf("GATEWAY_RETSIZE error\n");
        return(-1);
    }
    close(sd);
    sd = -1;
    return(rc);
}

int wait_for_client(int *sdp,char str[INET6_ADDRSTRLEN],int variant)
{
	struct sockaddr_in6 serveraddr, clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	int sdconn = -1;
    str[0] = 0;
	//get_lockid(0);
	while ( *sdp < 0 )
	{
		if ((*sdp = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		{
			perror("socket() failed");
			break;
		}
		/*if ( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,(char *)&on,sizeof(on)) < 0)
		{
			perror("setsockopt(SO_REUSEADDR) failed");
			close(sd);
			sd = -1;
			break;
		}*/
		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin6_family = AF_INET6;
		serveraddr.sin6_port   = htons(SERVER_PORT+variant);
		serveraddr.sin6_addr   = in6addr_any;
		if ( bind(*sdp,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0 )
		{
			printf("variant.%d\n",variant);
			perror("variant bind() failed");
			close(*sdp);
			*sdp = -1;
			sleep(30);
			continue;
		}
		if ( listen(*sdp, 300) < 0 )
		{
			perror("listen() failed");
			close(*sdp);
			*sdp = -1;
			break;
		}
	}
	//release_lockid(0);
	
	if ( *sdp < 0 )
		return(-1);
	else
	{
		if ((sdconn = accept(*sdp, NULL, NULL)) < 0)	// non blocking would be nice
		{
			perror("accept() failed");
			return(-1);
		}
		else
		{
			getpeername(sdconn, (struct sockaddr *)&clientaddr,&addrlen);
			if ( inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, INET6_ADDRSTRLEN) != 0 )
			{
                printf("variant.%d [Client address is %20s | Client port is %6d] sdconn.%d\n",variant,str,ntohs(clientaddr.sin6_port),sdconn);
			} else printf("Error getting client str\n");
		}
	}
	return(sdconn);
}

void *_server_loop(void *_args)
{
	struct server_request *req;
	int sd,variant,sdconn,expected,rc,bytesReceived,numreqs = 0;
	long xferred = 0;
    char clientip[INET6_ADDRSTRLEN],*ip;
	variant = *(int *)_args;
	req = malloc(65536);
    sd = -1;
	printf("Start server_loop.%d on port.%d\n",variant,SERVER_PORT+variant);
	while ( 1 )
	{
		usleep(10000);
		if ( (sdconn= wait_for_client(&sd,clientip,variant)) >= 0 )
		{
			expected = (int)65534;//sizeof(*req);// - sizeof(req->space));
			//printf("wait for req %d bytes from gateway.%d\n",expected,srcgateway);
			while ( 1 )
			{
				bytesReceived = 0;
				while ( bytesReceived < expected )
				{
					rc = (int)recv(sdconn,&((unsigned char *)req)[bytesReceived],expected - bytesReceived, 0);
					if ( rc <= 0 )
					{
						if ( rc < 0 )
							printf("recv() failed\n");
						else printf("The client closed the connection\n");
						break;
					}
					bytesReceived += rc;
					if ( bytesReceived >= sizeof(req->H) )
					{
                        if ( req->H.argsize < 65534 && expected != req->H.argsize )
                            printf("expected %d -> %d\n",expected,req->H.argsize);
						expected = req->H.argsize;
					}
				}
				if ( bytesReceived < expected )
				{
					printf("The client.%d closed the connection before all of the data was sent, got %d of %d\n",variant,bytesReceived,expected);
					break;
				}
                ip = clientip;
                if ( strncmp(clientip,"::ffff:",strlen("::ffff:")) == 0 )
                    ip += strlen("::ffff:");
                req->H.retsize = process_client_packet(variant,req,ip);
                if ( req->H.retsize > 0 )
                {
                    // printf("return %d\n",req->retsize);
                    if ( (rc = (int)send(sdconn,req,req->H.retsize,0)) < req->H.retsize )
                    {
                        printf("send() failed? rc.%d instead of %d\n",rc,req->H.retsize);
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
