
//  Created by jl777
//  MIT License
//

#ifndef gateway_jl777_h
#define gateway_jl777_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include "crypto_box.h"
#include "randombytes.h"
#include "guardians.h"

#define SESSION_CYCLE 10

#ifndef COINCONFIG
#define COINCONFIG "DOGE.h"
#endif
#include COINCONFIG

#ifdef __APPLE__
#define NXTSERVER "http://tn01.nxtsolaris.info:6876/nxt?requestType"
//#define NXTSERVER "http://209.126.73.160:6876/nxt?requestType"
#else
#ifdef MAINNET
#define NXTSERVER "http://localhost:7876/nxt?requestType"
#else
#define NXTSERVER "http://localhost:6876/nxt?requestType"
#endif
#endif

#ifdef MAINNET
#define NXTISSUERACCT "10154506025773104943"
#define NXTACCTA "10154506025773104943"
#define NXTACCTB "10154506025773104943"
#define NXTACCTC "10154506025773104943"
//#define NXTSERVER "http://localhost:7876/nxt?requestType"
#else
#define NXTISSUERACCT "18232225178877143084"
#define NXTACCTA "18232225178877143084"
#define NXTACCTB "182322251788771430841"
#define NXTACCTC "1823222517887714308412"
//#define NXTSERVER "https://holms.cloudapp.net:6875/nxt?requestType"
#endif

#ifndef GATEWAYID
#define GATEWAYID 1
#endif

#define SERVER_PORT 3005
#define SERVER_PORTSTR "3005"
#define INTERNAL_VARIANT(destgateway) ((destgateway)*NUM_GATEWAYS + GATEWAYID)
#define INTERNAL_MULTISYNC 0
#define GATEWAY_SIG 0xdadafeed

#define GENESISACCT "1739068987193023818"
#define SATOSHIDEN 100000000
#define MIN_NXTFEE 1
#define MIN_NXTCONFIRMS 1  // need to change this to 10
#define NXT_TOKEN_LEN 160
#define MAX_NXTTXID_LEN 32
#define MAX_NXTADDR_LEN 32
#define MAX_RAWINPUTS 16
#define MAX_COINTXID_LEN 160
#define MAX_COINADDR_LEN 64
#define MAX_VOUTS 8
#define DEPOSIT_FREQUENCY 3
#define POLL_SECONDS 60
#define MAX_ACTIVE_PEERS 20

// defines
// API funcids
#define CREATE_NXTCOINS 'C'
#define GET_COINDEPOSIT_ADDRESS 'g'
#define SET_COINWITHDRAW_ADDRESS 'w'
#define SEND_ANONYMOUS_PAYMENTS 'A'

// gateway internal funcids
#define START_NEW_SESSION 'N'
#define BIND_DEPOSIT_ADDRESS 'b'
#define PENDING_SWEEP 's'
#define WITHDRAW_REQUEST '<'
#define MONEY_SENT 'm'


#define NUM_GATEWAYS 3
#if GATEWAYID == 0
#define NXTACCT NXTACCTA
#define SERVER_NAME SERVER_NAMEA
#elif GATEWAYID == 1
#define NXTACCT NXTACCTB
#define IS_POOLSERVER
#define SERVER_NAME SERVER_NAMEB
#elif GATEWAYID == 2
#define NXTACCT NXTACCTC
#define SERVER_NAME SERVER_NAMEC
#else
illegal GATEWAYID, must define GATEWAYID to 0, 1 or 2
#endif

struct strings { char **list; void **argptrs,**argptrs2; int64_t *args,*arg2; int32_t num,max; };

struct hashtable
{
    char *name;
    void **hashtable;
    int64_t hashsize,numsearches,numiterations,numitems;
    long keyoffset,keysize,modifiedoffset,structsize;
};

struct payment_bundle
{
    unsigned char escrow_pubkey[crypto_box_PUBLICKEYBYTES];
    unsigned char depositaddr[MAX_NXTADDR_LEN];
    unsigned char paymentacct_key[crypto_box_SECRETKEYBYTES];
    unsigned char txouts[8][MAX_NXTADDR_LEN];
    int64_t amounts[8],sessionid;
};

struct NXTcoins_data    // 1% of presale and mined goes to pooling acct, send (authorized - .99*presale) to pooling acct
{
    int64_t totalcoins __attribute__ ((packed));    // in satoshis
    
    int64_t presale __attribute__ ((packed));       // in satoshis, must be less than authorized and exactly match preissued
    int64_t royalty __attribute__ ((packed));       // rate in satoshis, goes to issuer
    int64_t bountyrate __attribute__ ((packed));    // rate in satoshis, goes to bountyfund
    int64_t donation __attribute__ ((packed));      // rate in satoshis, goes to donationfund, defaulted to NXTcoins
    int64_t blockrewards[16][2] __attribute__ ((packed));    // in satoshis

    char coin_name[16];
    char website[64],sourcecode[64];
    char issuer[MAX_NXTADDR_LEN],poolingacct[MAX_NXTADDR_LEN],assetidstr[MAX_NXTADDR_LEN];
    char bountyfund[MAX_NXTADDR_LEN],presalefund[MAX_NXTADDR_LEN],donationfund[MAX_NXTADDR_LEN];
    
};

struct gateway_AM
{
    int32_t sig __attribute__ ((packed));
    int32_t funcid __attribute__ ((packed));
    int32_t gatewayid __attribute__ ((packed));
    int32_t coinid __attribute__ ((packed));
    int32_t timestamp __attribute__ ((packed));
    int32_t sessionid __attribute__ ((packed));
    int32_t vout __attribute__ ((packed));
    int32_t tbd __attribute__ ((packed));
    
    int64_t  amount __attribute__ ((packed));
    int64_t  unspent __attribute__ ((packed));
    int64_t  change __attribute__ ((packed));
    int64_t  quantity __attribute__ ((packed));
    char NXTaddr[MAX_NXTADDR_LEN],coinaddr[MAX_COINADDR_LEN];
    union
    {
        struct NXTcoins_data coin;
        struct
        {
            char txid[MAX_COINTXID_LEN],txid2[MAX_COINTXID_LEN],txid3[MAX_COINTXID_LEN],txid4[MAX_COINTXID_LEN];
            char token[NXT_TOKEN_LEN];
        };
        struct
        {
            unsigned char publickey[crypto_box_PUBLICKEYBYTES];
            unsigned char nonce[crypto_box_NONCEBYTES];
            unsigned char paymentkey_by_prevrecv[crypto_box_PUBLICKEYBYTES + crypto_box_SECRETKEYBYTES + crypto_box_ZEROBYTES];
            unsigned char payload_by_escrow[sizeof(struct payment_bundle) + crypto_box_ZEROBYTES];
        };
    };
};

struct gateway_state
{
    struct strings deposits,depositaddrs,multisig;
    char redeemscript[512],multisigaddr[MAX_COINTXID_LEN];
};

struct active_NXTacct
{
    int32_t counter,numdeposits,maxdeposits,numsweeps,numtransfers,numredemptions,numwithdraws,numassets;
    int64_t total_deposits,total_transfers,total_assets_redeemed,total_withdraws;
    int64_t pending_sweepamount,pending_transfer,pending_redeem,pending_withdraw;
    int32_t assetnonz,asseterrors,assetmissing,blacklist,submissions;
    int64_t *balances,nodeshares,nodecoins,current_nodecoins,nodecoins_sent;
    char NXTaddr[MAX_NXTADDR_LEN],withdrawaddr[MAX_COINADDR_LEN],redeem_txid[MAX_COINTXID_LEN],ipaddr[INET6_ADDRSTRLEN];
    struct gateway_state gsm[NUM_GATEWAYS];
};

struct NXT_trade { int64_t price,quantity; char *buyer,*seller,*txid; int timestamp,assetid; };

struct peer_info { int64_t uploaded,downloaded __attribute__ ((packed)); char ipaddr[INET6_ADDRSTRLEN]; } __attribute__ ((packed));

struct server_request_header { int32_t retsize,argsize,variant,funcid __attribute__ ((packed)); };
struct server_request
{
	struct server_request_header H __attribute__ ((packed));
    int32_t timestamp,arg,srcgateway,destgateway,numinputs,isforging __attribute__ ((packed));
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
    int32_t retsize,numips,numnxtaccts,tbd3;
    int64_t nodeshares,current_nodecoins,nodecoins,nodecoins_sent;
};

static char *Gateway_NXTaddrs[NUM_GATEWAYS] = { NXTACCTA, NXTACCTB, NXTACCTC };
static char *Gateway_Pubkeys[NUM_GATEWAYS] = { PUBLICA, PUBLICB, PUBLICC };
static char *Server_names[NUM_GATEWAYS+1] = { SERVER_NAMEA, SERVER_NAMEB, SERVER_NAMEC, "" };
static char NXTACCTSECRET[128]; // stored in plain text in RAM! suggest storing encrypted and decrypt only when needed
static int Forged_minutes,RTflag,BLOCK_ON_SERIOUS;

static int is_gateway_AM(int *gatewayidp,struct gateway_AM *ap,char *sender,char *receiver)
{
    int i;
    *gatewayidp = 0;
    if ( ap->sig == GATEWAY_SIG )
    {
        // good place to check for valid "website" token
        for (i=0; i<NUM_GATEWAYS; i++)
            if ( strcmp(sender,Gateway_NXTaddrs[i]) == 0 )
            {
                *gatewayidp = i;
                return(1);
            }
        if ( strcmp(sender,ap->NXTaddr) == 0 )
        {
            for (i=0; i<NUM_GATEWAYS; i++)
                if ( strcmp(receiver,Gateway_NXTaddrs[i]) == 0 )
                    *gatewayidp = i;
            if ( strcmp(sender,NXTISSUERACCT) == 0 )
                return(1);
            return(1);
        }
        printf("REJECTED AM from %s addr inside %s\n",sender,ap->NXTaddr);
    }
    return(0);
}

extern void update_gateway_states(int timestamp,struct gateway_AM *ap);
extern void queue_asset_redemption(int timestamp,int gatewayid,struct active_NXTacct *active,char *recipient,char *redeem_txid,int64_t redeemtoshis,char *assetid);
extern void update_asset_balances(char *assetname,char *change_txid,char *assetidstr,char *recipientstr,char *senderstr,int64_t satoshis);


#endif
