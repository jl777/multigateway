
//  Created by jl777, Feb 2014
//  MIT License
//
// gcc -o gateway_client gateway_client.c -lcurl
// ./gateway_client <NXT addr> <NXT passkey> <DOGE withdraw addr>

// BUGS:
// make sure acct has NXT!

#include "jl777.h"
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
    if ( am->funcid == BIND_DEPOSIT_ADDRESS )
    {
        printf("deposit address for gateway.%d %s is %s\n",gatewayid,am->NXTaddr,am->coinaddr);
        strcpy(DEPOSITADDR,ap->coinaddr);
    }
}

void gateway_client(int gatewayid,char *nxtaddr,char *withdrawaddr)
{
    static char lastblock[256] = "";
    int timestamp = 0;
    char *blockidstr,*depositaddr = 0;
    printf("Get gateway.%d deposit address for %s and set withdraw address to %s\n",gatewayid,nxtaddr,withdrawaddr);
    AM_get_coindeposit_address(timestamp,gatewayid,nxtaddr,withdrawaddr,0);
    while ( WITHDRAWADDR[0] == 0 || DEPOSITADDR[0] == 0 )
    {
        blockidstr = issue_getState();
        //printf("block.(%s) vs lastblock.(%s)\n",blockidstr,lastblock);
        if ( blockidstr != 0 && strcmp(blockidstr,lastblock) != 0 )
        {
            issue_getBlock((blockiterator)process_NXTtransaction,blockidstr);
            printf("NEW block.(%s) vs lastblock.(%s)\n",blockidstr,lastblock);
            strcpy(lastblock,blockidstr);
        }
        sleep(POLL_SECONDS);
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
    gateway_client(gatewayid,NXTADDR,WITHDRAWADDR);
    printf("\n\n>>>>> gateway.%d deposit address for %s is %s and withdraw address is %s\n",gatewayid,NXTADDR,DEPOSITADDR,WITHDRAWADDR);
    return(0);
}
