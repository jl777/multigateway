//  Created by jl777, Feb 2014
//  MIT License
//
// gcc -o gateway_client gateway_client.c -lcurl
// ./gateway_client <NXT addr> <NXT passkey> <DOGE withdraw addr>

// BUGS:
// make sure acct has NXT!

#include "jl777.h"
//#include "NXTcrypto.h"
#include "NXTutils.h"
#include "NXTparse.h"
#include "NXTAPI.h"
#include "NXTassets.h"

// globals
char WITHDRAWADDR[64],DEPOSITADDR[64],NXTADDR[64];

char *AM_get_coindeposit_address(int timestamp,int gatewayid,char *nxtaddr,char *withdrawaddr,char *userpubkey)
{
    struct gateway_AM AM;
    strcpy(WITHDRAWADDR,withdrawaddr);
    set_standard_AM(&AM,GET_COINDEPOSIT_ADDRESS,nxtaddr,timestamp);
    AM.gatewayid = gatewayid;
    if ( withdrawaddr != 0 )
        strcpy(AM.txid,withdrawaddr);
    if ( userpubkey != 0 )
        strcpy(AM.txid2,userpubkey);
    return(submit_AM(&AM));
}

void update_gateway_states(int timestamp,struct gateway_AM *am)
{
    int i;
    if ( am->funcid == BIND_DEPOSIT_ADDRESS )
    {
        printf("deposit address for gateway.%d %s is %s\n",am->gatewayid,am->NXTaddr,am->coinaddr);
        strcpy(DEPOSITADDR,am->coinaddr);
    }
    else if ( am->funcid == START_NEW_SESSION )
    {
        for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
            printf("%02x ",am->publickey[i]);
        printf("NEW SESSION.%d\n",am->sessionid);
        //memcpy(ESCROW_PUBLICKEYS[am->sessionid % MAX_SESSIONS],am->publickey,crypto_box_PUBLICKEYBYTES);
        //broadcast_anonymous(timestamp,am->sessionid,NXTADDR);
    }
}

int process_NXTtransaction(char *nxt_txid)
{
    static int timestamp = 0;
    struct gateway_AM AM;
    struct active_NXTacct *active;
    char cmd[4096],*jsonstr,*retstr,*cmpstr,*assetidstr,*sender,*recv,*assetname;
    int gatewayid,n,tmp,flag = 0;
    char tmpstr[10000];
    int64_t mult,quantity;
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,nxt_txid);
    //printf("CMD.(%s)\n",cmd);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("PROCESS.(%s)\n",jsonstr);
        strcpy(tmpstr,jsonstr);
        retstr = parse_NXTresults(0,"message","",results_processor,jsonstr,strlen(jsonstr));
        //printf("MESSAGE[%s]\n",retstr);
        tmp = atoi(Timestamp);
        if ( tmp > timestamp )
            timestamp = tmp;
        n = (int)strlen(Message) / 2;
        if ( retstr != 0 && retstr[0] != 0 && n >= 10 )
        {
            //printf("retstr.(%s)\n",retstr);
            memset(&AM,0,sizeof(AM));
            decode_hex((void *)&AM,n,Message);
            //for (j=0; j<n; j++)
            //    printf("%02x",((char *)&AM)[j]&0xff);
            //printf("timestamp.%s %d\n",Timestamp,timestamp);
            if ( is_gateway_AM(&gatewayid,&AM,Sender,Recipient) != 0 )
            {
                flag++;
                update_gateway_states(timestamp,&AM);
            }
            free(retstr);
        }
        free(jsonstr);
        printf("DONE WITH ITER\n");
    }
    return(timestamp);
}

void gateway_client(int gatewayid,char *nxtaddr,char *withdrawaddr)
{
    static char lastblock[256] = "";
    int timestamp = 0;
    char *blockidstr,*depositaddr = 0;
    printf("Get gateway.%d deposit address for %s and set withdraw address to %s\n",gatewayid,nxtaddr,withdrawaddr);
    AM_get_coindeposit_address(timestamp,gatewayid,nxtaddr,withdrawaddr,0);
    while ( 1 )//WITHDRAWADDR[0] == 0 || DEPOSITADDR[0] == 0 )
    {
        blockidstr = issue_getState();
        //printf("block.(%s) vs lastblock.(%s)\n",blockidstr,lastblock);
        if ( blockidstr != 0 && strcmp(blockidstr,lastblock) != 0 )
        {
            issue_getBlock((blockiterator)process_NXTtransaction,blockidstr);
            printf("NEW block.(%s) vs lastblock.(%s)\n",blockidstr,lastblock);
            strcpy(lastblock,blockidstr);
        }
        sleepOS(POLL_SECONDS);
    }
}

int main(int argc, const char * argv[])
{
    int gatewayid = 0;
    if ( argc < 4 )
    {
        printf("usage: %s <NXT addr> <NXT acct passkey> <coin withdraw addr> [gatewayid]\n",argv[0]);
        return(-1);
    }
    strcpy(NXTADDR,argv[1]);
    strcpy(NXTACCTSECRET,argv[2]);
    strcpy(WITHDRAWADDR,argv[3]);
    if ( argc > 4 )
    {
        gatewayid = atoi(argv[4]);
        if ( gatewayid < 0 || gatewayid >= NUM_GATEWAYS )
            gatewayid = 0;
    }

#ifdef _WIN32
	winsock_startup();
#endif 

    gateway_client(gatewayid,NXTADDR,WITHDRAWADDR);
	printf("\n\n>>>>> gateway.%d deposit address for %s is %s and withdraw address is %s\n",gatewayid,NXTADDR,DEPOSITADDR,WITHDRAWADDR);

#ifdef _WIN32
	winsock_cleanup();
#endif 

	return(0);
}
