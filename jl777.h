//  Created by jl777
//  MIT License
//

#ifndef gateway_jl777_h
#define gateway_jl777_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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
#define GATEWAY_RETSIZE 0
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

// defines
// API funcids
#define GET_COINDEPOSIT_ADDRESS 'g'
#define SET_COINWITHDRAW_ADDRESS 'w'

// gateway internal funcids
#define BIND_DEPOSIT_ADDRESS 'b'
#define PENDING_SWEEP 's'
#define WITHDRAW_REQUEST '<'
#define MONEY_SENT 'm'

// typedefs

#define NUM_GATEWAYS 3
#if GATEWAYID == 0
#define NXTACCT NXTACCTA
#define SERVER_NAME SERVER_NAMEA
#elif GATEWAYID == 1
#define NXTACCT NXTACCTB
#define SERVER_NAME SERVER_NAMEB
#elif GATEWAYID == 2
#define NXTACCT NXTACCTC
#define SERVER_NAME SERVER_NAMEC
#else
illegal GATEWAYID, must define GATEWAYID to 0, 1 or 2
#endif

struct strings { char **list; void **argptrs,**argptrs2; int64_t *args,*arg2; int num,max; };

struct gateway_AM
{
    int32_t sig __attribute__ ((packed));
    int32_t funcid __attribute__ ((packed));
    int32_t gatewayid __attribute__ ((packed));
    int32_t coinid __attribute__ ((packed));
    int32_t timestamp __attribute__ ((packed));   // used as nonce
    int32_t tbd2 __attribute__ ((packed));
    int32_t vout __attribute__ ((packed));
    int32_t tbd __attribute__ ((packed));
    
    int64_t  amount __attribute__ ((packed));
    int64_t  unspent __attribute__ ((packed));
    int64_t  change __attribute__ ((packed));
    int64_t  quantity __attribute__ ((packed));
    char NXTaddr[MAX_NXTADDR_LEN],coinaddr[MAX_COINADDR_LEN];
    char txid[MAX_COINTXID_LEN],txid2[MAX_COINTXID_LEN],txid3[MAX_COINTXID_LEN],txid4[MAX_COINTXID_LEN];
    char token[NXT_TOKEN_LEN];
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
    int64_t *balances;
    char NXTaddr[MAX_NXTADDR_LEN],withdrawaddr[MAX_COINADDR_LEN],redeem_txid[MAX_COINTXID_LEN];
    struct gateway_state gsm[NUM_GATEWAYS];
};

static char *Gateway_NXTaddrs[NUM_GATEWAYS] = { NXTACCTA, NXTACCTB, NXTACCTC };
static char *Gateway_Pubkeys[NUM_GATEWAYS] = { PUBLICA, PUBLICB, PUBLICC };
static char *Server_names[NUM_GATEWAYS] = { SERVER_NAMEA, SERVER_NAMEB, SERVER_NAMEC };

#endif
