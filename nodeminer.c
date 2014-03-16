
//  Created by jl777, Feb 2014
//  MIT License
//
// gcc -o nodeminer nodeminer.c -lcurl
// ./nodeminer <NXT addr> <NXT passkey> <DOGE withdraw addr>

// BUGS:
// make sure acct has NXT!

// globals
char WITHDRAWADDR[64],DEPOSITADDR[64],NXTADDR[64];
int Deadman_switch;

#include "jl777.h"
//#include "NXTcrypto.h"
#include "NXTutils.h"
#include "NXTsock.h"
#include "NXTparse.h"
#include "NXTAPI.h"
#include "NXTassets.h"
//#include "nodecoin.h"
#define GETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define NODECOIN_VARIANT 0
#define NODECOIN_SUBMITPEERS 0
struct strings PEERS;


char *AM_get_coindeposit_address(int timestamp,int gatewayid,char *nxtaddr,char *withdrawaddr,char *userpubkey)
{
    struct gateway_AM AM;
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
    }
    return(timestamp);
}

char *choose_poolserver(char *NXTaddr)
{
    uint64_t hashval;
    //hashval = calc_decimalhash(NXTaddr);
    //return(Guardian_names[hashval % Numguardians]);
    return(SERVER_NAMEA);
}
// nodeminer client

char *issue_getPeer(char *peer)
{
    char cmd[4096],*jsonstr,*retstr=0;
    sprintf(cmd,"%s=getPeer&peer=%s",NXTSERVER,peer);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getPeer.(%s) -> (%s)\n\n",peer,jsonstr);
        retstr = parse_NXTresults(0,"announcedAddress",0,results_processor,jsonstr,strlen(jsonstr));
        //if ( retstr != 0 )
        //    printf("announced.(%s)\n",retstr);
        myfree(jsonstr,"141");
    }
    return(retstr);
}

int issue_getPeers(struct peer_info peers[MAX_ACTIVE_PEERS])
{
    int j,c,peerid,numpeers = 0;
    unsigned int downloaded,uploaded;
    char cmd[4096],*jsonstr,*str,addr[4096],*retstr;
    memset(peers,0,sizeof(*peers) * MAX_ACTIVE_PEERS);
    sprintf(cmd,"%s=getPeers",NXTSERVER);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getPeers.(%s)\n\n",jsonstr);
        if ( (str= strstr(jsonstr,"\"peers\":[")) != 0 )
        {
            str += strlen("\"peers\":[");
            j = 0;
            addr[0] = 0;
            while ( *str != ']' )
            {
                //printf("[%s]\n",str);
                if ( (c= *str++) == ',' || c == '}' )
                {
                    addr[j] = 0;
                    if ( addr[0] == '"' && addr[strlen(addr)-1] == '"' )
                    {
                        addr[strlen(addr)-1] = 0;
                        //printf("(%s) ",addr+1);
                        retstr = issue_getPeer(addr+1);
                        //(209.126.73.162) getPeer.(209.126.73.162) -> ({"shareAddress":true,"platform":null,"application":null,"weight":0,"state":0,"announcedAddress":"209.126.73.162","downloadedVolume":0,"blacklisted":false,"version":null,"uploadedVolume":0})
                        
                        if ( retstr != 0 )
                        {
                            if ( atoi(State) == 1 && strcmp(Blacklisted,"false") == 0 && strcmp(ShareAddress,"true") == 0 && AnnouncedAddress[0] != 0 && (downloaded= atoi(DownloadedVolume)) > 0 && (uploaded= atoi(UploadedVolume)) > 0 )
                            {
                                if ( (peerid= find_string(&PEERS,addr+1)) < 0 )
                                    add_string(&PEERS,addr+1,downloaded,uploaded,0,0);
                                if ( (peerid= find_string(&PEERS,addr+1)) >= 0 )
                                {
                                    printf("%d: GOOD PEER.%d (%.0f %.0f) %s ",numpeers,peerid,(double)(downloaded-PEERS.args[peerid]),(double)(uploaded-PEERS.arg2[peerid]),addr+1);
                                    printf("state.%s blacklist.%s share.%s announce.%s downloaded.%.0f uploaded.%.0f\n",State,Blacklisted,ShareAddress,AnnouncedAddress,(double)downloaded,(double)uploaded);
                                    struct peer_info { int64_t uploaded,downloaded; char ipaddr[16]; };
                                    if ( numpeers < MAX_ACTIVE_PEERS )
                                    {
                                        strncpy(peers[numpeers].ipaddr,addr+1,sizeof(peers[numpeers].ipaddr) - 1);
                                        peers[numpeers].downloaded = (downloaded - PEERS.args[peerid]);
                                        peers[numpeers].uploaded = (uploaded - PEERS.arg2[peerid]);
                                        numpeers++;
                                    }
                                }
                            }
                            myfree(retstr,"113");
                        }
                    }
                    addr[0] = 0;
                    j = 0;
                }
                else addr[j++] = c;
            }
        }
        myfree(jsonstr,"41");
    }
    return(numpeers);
}

void nodecoin_loop(char *NXTaddr,int loopflag)
{
    char *retstr,*destserver;
    int i,peerid,numactivepeers;
    struct server_request R;
    struct server_response *rp = (struct server_response *)&R;
    struct peer_info peers[MAX_ACTIVE_PEERS];
    destserver = choose_poolserver(NXTaddr);
    while ( 1 )
    {
        if ( (retstr= issue_getState()) != 0 )
        {
            myfree(retstr,"122");
            if ( atoi(NumberOfUnlockedAccounts) <= 0 )
                issue_startForging();
        }
        numactivepeers = issue_getPeers(peers);
        if ( numactivepeers > 0 )
        {
            printf("numactivepeers.%d\n",numactivepeers);
            memset(&R,0,sizeof(R));
            strcpy(R.NXTaddr,NXTaddr);
            R.H.argsize = sizeof(R);
            memcpy(R.peers,peers,sizeof(peers));
            if ( server_request(destserver,&R.H,NODECOIN_VARIANT,NODECOIN_SUBMITPEERS) == sizeof(struct server_response) )
            {
                for (i=0; i<MAX_ACTIVE_PEERS; i++)
                {
                    if ( peers[i].ipaddr[0] == 0 )
                        continue;
                    if ( (peerid= find_string(&PEERS,peers[i].ipaddr)) >= 0 ) // update xfer cutoff, this punishes unavailables
                    {
                        PEERS.args[peerid] += peers[i].downloaded;
                        PEERS.arg2[peerid] += peers[i].uploaded;
                        printf("(%s %.0f %.0f) ",peers[i].ipaddr,(double)peers[i].uploaded,(double)peers[i].downloaded);
                    } else printf("cant find peer.(%s)?\n",peers[i].ipaddr);
                }
                printf("%ld shares, current %.8f %.8f nodecoins | sent %.8f\n",(long)rp->nodeshares,(double)rp->current_nodecoins/SATOSHIDEN,(double)rp->nodecoins/SATOSHIDEN,(double)rp->nodecoins_sent/SATOSHIDEN);
            }
            else printf("error submitting results to (%s)\n",choose_poolserver(NXTaddr));
        }
        if ( loopflag == 0 )
            break;
        sleep(10);
    }
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
            nodecoin_loop(NXTADDR,0);
        }
        sleep(POLL_SECONDS);
    }
}

int main(int argc, const char * argv[])
{
    FILE *fp;
    char *ipaddr;
    unsigned char bits[32];
    int i,j,x,gatewayid = 0;
    if ( argc < 3 )
    {
        fp = fopen("randvals","rb");
        if ( fp == 0 )
            system("dd if=/dev/urandom count=32 bs=1 > randvals");
        fp = fopen("randvals","rb");
        if ( fp == 0 )
        {
            ipaddr = issue_getMyInfo();
            if ( ipaddr != 0 )
            {
                strcpy(NXTACCTSECRET,ipaddr);
                strcpy(NXTADDR,issue_getAccountId(ipaddr));
                free(ipaddr);
            }
            else
            {
                printf("usage: %s <NXT addr> <NXT acct passkey> <coin withdraw addr> [gatewayid]\n",argv[0]);
                return(-1);
            }
        }
        else
        {
            fread(bits,1,sizeof(bits),fp);
            for (i=0; i+6<(sizeof(bits)*8); i+=6)
            {
                for (j=x=0; j<6; j++)
                {
                    if ( GETBIT(bits,i*6+j) != 0 )
                        x |= (1 << j);
                }
                //printf("i.%d j.%d x.%d %c\n",i,j,x,'0'+x);
                NXTACCTSECRET[i/6] = '0' + x;
            }
            NXTACCTSECRET[i/6] = 0;
            strcpy(NXTADDR,issue_getAccountId(NXTACCTSECRET));
            printf("NXTADDR.%s (%s)\n",NXTADDR,NXTACCTSECRET);
            fclose(fp);
        }
    }
    else
    {
        strcpy(NXTADDR,argv[1]);
        strcpy(NXTACCTSECRET,argv[2]);
    }
    if ( argc > 3 )
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
