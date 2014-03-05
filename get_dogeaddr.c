//  Created by jl777, Feb 2014
//  MIT License
//
// gcc -o get_dogeaddr get_dogeaddr.c -lcurl
// ./get_dogeaddr <NXT addr> <NXT passkey> <DOGE withdraw addr> [gatewayid]

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <math.h>
#include <curl/curl.h>
#include <curl/easy.h>


#define MAX_TOKEN_LEN 4096

//#define MANUAL_THRESHOLD  100000  // any transactions over this size require manual confirmation, by all gateways
#define COINID 1
#define COINNAME "DOGE"
#define DAEMON "dogecoind"  // for most bitcoin forks, changing TXFEE and gateway specific defines should be enough
#define TXFEE 1.0           // don't forget to match txfee with coin
#define MAX_VOUTS 8
#define MIN_CONFIRMS 3
#define DEPOSIT_FREQUENCY 3
#define POLL_SECONDS 10
#define WALLETBACKUP "wallet.DOGE"
#define COINASSET "7761388364129412234"
#define milliCOINASSET "6572437125760810791"

// defines
#define MULTISIGADDR "9rufqRXAekAx4gnGuKf5SL8YzViMfryLMY"    // this and redeem are deterministic based on signers
#define REDEEMSCRIPT "52210287300d3f7447c84c0ec6052440285d99f2211aba6eb17b9d6baa9b62cd16f8d52102b6b6aa9320c60d47b795748ae5291e94f56267411096f8ea42d6f7202ba0819c2102daa52056d79777afa25001b957d9e4cc2eba1e3639bd390681db149f1cd174fb53ae"

// generate publickeys using validateaddress on the address getaccountaddress generates
#define SERVER_NAMEA "209.126.71.170"
#define DOGEADDRA "DQaHT9CaHnAcqHvNui7V6j8fAh3CD21wpx"
#define PUBLICA "0287300d3f7447c84c0ec6052440285d99f2211aba6eb17b9d6baa9b62cd16f8d5"

#define SERVER_NAMEB "209.126.73.156"
#define DOGEADDRB "DURjNgaqPUEjY3Nk2CMqkRccNoouWXm6mx"
#define PUBLICB "02b6b6aa9320c60d47b795748ae5291e94f56267411096f8ea42d6f7202ba0819c"

#define SERVER_NAMEC "209.126.73.158"
#define DOGEADDRC "DUJxdKGYrThiuWMyuw3oGkfVnydAM5hG46"
#define PUBLICC "02daa52056d79777afa25001b957d9e4cc2eba1e3639bd390681db149f1cd174fb"

#ifdef MAINNET
#define NXTISSUERACCT "10154506025773104943"
#define NXTACCTA "10154506025773104943"
#define NXTACCTB "10154506025773104943"
#define NXTACCTC "10154506025773104943"
#define NXTSERVER "http://localhost:7874/nxt?requestType"
#else
#define NXTISSUERACCT "18232225178877143084"
#define NXTACCTA "18232225178877143084"
#define NXTACCTB "182322251788771430841"
#define NXTACCTC "1823222517887714308412"
//#define NXTSERVER "https://holms.cloudapp.net:6875/nxt?requestType"
#define NXTSERVER "http://tn01.nxtsolaris.info:6876/nxt?requestType"
//#define NXTSERVER "http://209.126.73.160:6876/nxt?requestType"
#endif


// defines
// API funcids
#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'

// typedefs
typedef void *(*funcp)(char *field,char *arg,char *keyname);
typedef char *(*blockiterator)(char *blockidstr);

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

char *Gateway_NXTaddrs[NUM_GATEWAYS] = { NXTACCTA, NXTACCTB, NXTACCTC };
char *Gateway_Pubkeys[NUM_GATEWAYS] = { PUBLICA, PUBLICB, PUBLICC };
char *Server_names[NUM_GATEWAYS] = { SERVER_NAMEA, SERVER_NAMEB, SERVER_NAMEC };

#define SERVER_PORT 3005
#define SERVER_PORTSTR "3005"
#define GATEWAY_RETSIZE 0
#define GATEWAY_SIG 0xdadafeed

#define SATOSHIDEN 100000000
#define MIN_NXTFEE 1
#define MIN_NXTCONFIRMS 10  // need to implement this!
#define NXT_TOKEN_LEN 160
#define MAX_NXTTXID_LEN 32
#define MAX_NXTADDR_LEN 32
#define MAX_RAWINPUTS 16
#define MAX_COINTXID_LEN 160
#define MAX_COINADDR_LEN 64

struct gateway_AM
{
    int32_t sig __attribute__ ((packed));
    int32_t funcid __attribute__ ((packed));
    int32_t gatewayid __attribute__ ((packed));
    int32_t coinid __attribute__ ((packed));
    int32_t timestamp __attribute__ ((packed));   // used as nonce
    int32_t quantity __attribute__ ((packed));
    int32_t vout __attribute__ ((packed));
    int32_t tbd __attribute__ ((packed));
    
    int64_t  amount __attribute__ ((packed));
    int64_t  unspent __attribute__ ((packed));
    int64_t  change __attribute__ ((packed));
    int64_t  tbdl __attribute__ ((packed));
    char NXTaddr[MAX_NXTADDR_LEN],coinaddr[MAX_COINADDR_LEN];
    char txid[MAX_COINTXID_LEN],txid2[MAX_COINTXID_LEN],txid3[MAX_COINTXID_LEN],txid4[MAX_COINTXID_LEN];
    char token[NXT_TOKEN_LEN];
};


// globals
int Forged_minutes,Numtransactions,RTflag;
struct MemoryStruct { char *memory; size_t size; };
char NXTACCTSECRET[512];
char Sender[MAX_TOKEN_LEN],Block[MAX_TOKEN_LEN],Timestamp[MAX_TOKEN_LEN],Deadline[MAX_TOKEN_LEN];
char Quantity[MAX_TOKEN_LEN],Asset[MAX_TOKEN_LEN],Recipient[MAX_TOKEN_LEN],Amount[MAX_TOKEN_LEN];
char Fee[MAX_TOKEN_LEN],Confirmations[MAX_TOKEN_LEN],Signature[MAX_TOKEN_LEN],Bytes[MAX_TOKEN_LEN];
char Transaction[MAX_TOKEN_LEN],ReferencedTransaction[MAX_TOKEN_LEN],Subtype[MAX_TOKEN_LEN];
char Message[MAX_TOKEN_LEN],SenderPublicKey[MAX_TOKEN_LEN],Type[MAX_TOKEN_LEN],Description[MAX_TOKEN_LEN];
char WITHDRAWADDR[64],DEPOSITADDR[64],NXTADDR[64];

int unhex(char c)
{
    if ( c >= '0' && c <= '9' )
        return(c - '0');
    else if ( c >= 'a' && c <= 'f' )
        return(c - 'a' + 10);
    else return(0);
}

unsigned char _decode_hex(char *hex)
{
    return((unhex(hex[0])<<4) | unhex(hex[1]));
}

void decode_hex(unsigned char *bytes,int n,char *hex)
{
    int i;
    for (i=0; i<n; i++)
        bytes[i] = _decode_hex(&hex[i*2]);
}

char hexbyte(int c)
{
    if ( c < 10 )
        return('0'+c);
    else return('a'+c-10);
}

int init_hexbytes(char *hexbytes,unsigned char *message,long len)
{
    int i,lastnonz = -1;
    for (i=0; i<len; i++)
    {
        if ( message[i] != 0 )
        {
            lastnonz = i;
            hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
            hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
        }
        else hexbytes[i*2] = hexbytes[i*2+1] = '0';
        //printf("i.%d (%02x) [%c%c] last.%d\n",i,message[i],hexbytes[i*2],hexbytes[i*2+1],lastnonz);
    }
    lastnonz++;
    hexbytes[lastnonz*2] = 0;
    return(lastnonz*2+1);
}

void reset_strings()
{
    Sender[0] = Block[0] = Timestamp[0] = Deadline[0] = Quantity[0] = Asset[0] = Description[0] =
    Recipient[0] = Amount[0] = Fee[0] = Confirmations[0] = Signature[0] = Bytes[0] = Transaction[0] = 0;
    ReferencedTransaction[0] = Subtype[0] = Message[0] = SenderPublicKey[0] = Type[0] = 0;
}

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

char *issue_curl(char *arg)
{
    CURL *curl_handle;
    CURLcode res;
    // from http://curl.haxx.se/libcurl/c/getinmemory.html
    struct MemoryStruct chunk;
    chunk.memory = malloc(4096);  // will be grown as needed by the realloc above
    chunk.size = 0;    // no data at this point
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYHOST,0);
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYPEER,0);
    curl_easy_setopt(curl_handle, CURLOPT_URL, arg); // specify URL to get
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); // send all data to this function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk); // we pass our 'chunk' struct to the callback function
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // some servers don't like requests that are made without a user-agent field, so we provide one
    res = curl_easy_perform(curl_handle);
    if ( res != CURLE_OK )
        fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
    else
    {
        // printf("%lu bytes retrieved [%s]\n", (int64_t )chunk.size,chunk.memory);
    }
    curl_easy_cleanup(curl_handle);
    return(chunk.memory);
}

int64_t  stripstr(char *buf,int64_t  len)
{
    int i,j;
    for (i=j=0; i<len; i++)
    {
        buf[j] = buf[i];
        if ( buf[j] != ' ' && buf[j] != '\n' && buf[j] != '\r' && buf[j] != '\t' )
            j++;
    }
    buf[j] = 0;
    return(j);
}

int normal_parse(double *amountp,char *buf,int j)
{
    int i,isfloat = 0;
    char *token,str[4096];
    if ( buf[j] >= '0' && buf[j] <= '9' )
    {
        for (i=0; i<1000; i++)
        {
            str[i] = buf[j+i];
            if ( buf[j+i] == '.' )
            {
                isfloat = 1;
                continue;
            }
            if ( buf[j+i] < '0' || buf[j+i] > '9' )
                break;
        }
        str[i] = 0;
        //if ( isfloat != 0 )
        *amountp = atof(str);
        if ( buf[i+j] == '}' )
            j++;
        //else *amountp = atol(str);
        //printf("naked number (%f) <- (%s).%d i.%d j.%d\n",*amountp,str,isfloat,i,j);
        return(i+j);
    }
    if ( strncmp(buf+j,"{\"asset\":",9) == 0 )
        j += 9;
    if ( buf[j] != '"' )
    {
        printf("missing open double quote (%c) at j.%d (%s)\n",buf[j],j,buf);
        return(-1);
    }
    j++;
    token = buf+j;
    for (i=0; i<4000; i++)
        if ( buf[j+i] == '"' )
            break;
    if ( buf[j+i] != '"' )
    {
        token[100] = 0;
        printf("missing terminating double quote at j.%d [%s]\n",j,token);
        return(-1);
    }
    else
    {
        buf[j+i] = 0;
        j += i + 1;
        *amountp = atof(token);
    }
    return(j);
}

char *decode_json(char **tokenp,char *buf)  // returns ptr to "value"
{
    int j;
    double amount;
    j = 0;
    *tokenp = 0;
    if ( buf[j] == '{' )
    {
        j++;
        if ( buf[j] == '}' )
            return(0);
        else if ( buf[j] == '"' )
        {
            (*tokenp) = buf+j+1;
            j = normal_parse(&amount,buf,j);
            if ( j <= 0 )
            {
                printf("decode_json error (%s)\n",buf);
                return(0);
            }
            return(buf + j);
        }
    }
    else if ( buf[j] == '"' )
    {
        *tokenp = buf+j+1;
        j = normal_parse(&amount,buf,j);
        if ( j <= 0 )
        {
            printf("decode_json error2 (%s)\n",buf);
            return(0);
        }
        return(buf + j);
    }
    return(0);
}

void *results_processor(char *field,char *arg,char *keyname)
{
    static int successflag,amount;
    static char *resultstr;
    int i,isforging;
    char *retstr = 0;
    char argstr[4096];
    if ( arg != 0 )
    {
        for (i=0; i<4096; i++)
        {
            if ( arg[i] == 0 )
                break;
            if ( (argstr[i]= arg[i]) == ',' || arg[i] == '"' )
                break;
        }
    } else i = 0;
    argstr[i] = 0;
    if ( field != 0 )
    {
        if ( strcmp("signature",field) == 0 )
            strcpy(Signature,argstr);
        else if ( strcmp("asset",field) == 0 )
            strcpy(Asset,argstr);
        else if ( strcmp("quantity",field) == 0 )
            strcpy(Quantity,argstr);
        else if ( strcmp("fee",field) == 0 )
            strcpy(Fee,argstr);
        else if ( strcmp("confirmations",field) == 0 )
            strcpy(Confirmations,argstr);
        else if ( strcmp("block",field) == 0 )
            strcpy(Block,argstr);
        else if ( strcmp("timestamp",field) == 0 )
            strcpy(Timestamp,argstr);
        else if ( strcmp("referencedTransaction",field) == 0 )
            strcpy(ReferencedTransaction,argstr);
        else if ( strcmp("subtype",field) == 0 )
            strcpy(Subtype,argstr);
        else if ( strcmp("message",field) == 0 )
            strcpy(Message,argstr);
        else if ( strcmp("senderPublicKey",field) == 0 )
            strcpy(SenderPublicKey,argstr);
        else if ( strcmp("type",field) == 0 )
            strcpy(Type,argstr);
        else if ( strcmp("deadline",field) == 0 )
            strcpy(Deadline,argstr);
        else if ( strcmp("sender",field) == 0 )
            strcpy(Sender,argstr);
        else if ( strcmp("recipient",field) == 0 )
            strcpy(Recipient,argstr);
        else if ( strcmp("amount",field) == 0 )
            strcpy(Amount,argstr);
        else if ( strcmp("bytes",field) == 0 )
            strcpy(Bytes,argstr);
        else if ( strcmp("transaction",field) == 0 )
            strcpy(Transaction,argstr);
    }
    if ( field == 0 )
    {
        //printf("successflag.%d amount.%d resultstr.%s\n",successflag,amount,resultstr);
        if ( successflag > 0 )// || (successflag == 1 && amount != 0) )
            retstr = resultstr;
        resultstr = 0;
        amount = 0;
        successflag = 0;
        return(retstr);
    }
    else if ( strcmp(keyname,field) == 0 )
    {
        resultstr = arg;
        successflag = 1;
    }
    else
    {
#if NODESERVER == 0
        if ( strcmp("numberOfUnlockedAccounts",field) == 0 )
        {
            isforging = atoi(argstr);
            if ( isforging > 0 )
            {
                //Forged_minutes++;
                //printf("FORGING.%d ",Forged_minutes);
            }
        }
        //printf("[%s %s] success.%d\n",field,argstr,successflag);
#endif
    }
    return(retstr);
}

char *finalize_processor(funcp processor)
{
    int n;
    char *resultstr,*token;
    resultstr = (*processor)(0,0,0);
    if ( resultstr != 0 )
    {
        n = (int)strlen(resultstr);
        if ( n > 0 )
        {
            token = malloc(n+1);
            memcpy(token,resultstr,n);
            token[n] = 0;
            //printf("return (%s)\n",token);
        }
        else token = 0;
        return(token);
    }
    else return(0);
}

char *parse_NXTresults(blockiterator iterator,char *keyname,char *arrayfield,funcp processor,char *results,long len)
{
    char tmpstr[10000];
    int j,n;
    double amount;
    char *token,*valuestr,*field,*fieldvalue,*blockidstr;
    if ( results == 0 )
        return(0);
    strcpy(tmpstr,results);
    reset_strings();
    (*processor)(0,0,0);
    len = stripstr(results,len);
    if ( len == 0 )
        return(0);
    else if ( results[0] == '{' )
        valuestr = results+1;
    else valuestr = results;
    n = 0;
    fieldvalue = valuestr;
    while ( valuestr[0] != 0 && valuestr[0] != '}' )
    {
        fieldvalue = decode_json(&field,valuestr);
        if ( fieldvalue == 0 || field == 0 )
        {
            printf("field error.%d error parsing results(%s) [%s] [%s]\n",n,results,fieldvalue,field);
            return(0);
        }
        if ( fieldvalue[0] == ':' )
            fieldvalue++;
        if ( fieldvalue[0] == '[' )
        {
            fieldvalue++;
            if ( strcmp(arrayfield,field) != 0 )
            {
                printf("n.%d unexpected nested fieldvalue0 %s for field %s\n",n,fieldvalue,field);
                return(0);
            }
            while ( fieldvalue[0] != ']' )
            {
                j = normal_parse(&amount,fieldvalue,0);
                if ( j <= 0 )
                {
                    printf("decode_json error (%s)\n",fieldvalue);
                    return(0);
                }
                if ( iterator != 0 )
                {
                    char argstr[64],i,j;
                    i = 0;
                    if ( fieldvalue[i] == '"' )
                        i++;
                    for (j=0; i<64; i++)
                        if ( (argstr[j++]= fieldvalue[i]) == ',' || fieldvalue[i] == '"' )
                            break;
                    argstr[j] = 0;
                    blockidstr = fieldvalue + (fieldvalue[0]=='"'?1:0);
                    (*iterator)(blockidstr);
                    //printf("(%s.%d %s)\n",field,n,blockidstr);
                }
                fieldvalue += j;
                if ( fieldvalue[0] == ',' )
                    fieldvalue++;
                n++;
            }
            valuestr = ++fieldvalue;
            if ( valuestr[0] == ',' )
                valuestr++;
            //printf("<%s> ",valuestr);
        }
        else
        {
            // printf("[%s]\n",fieldvalue);
            //printf("FIELD.(%s) FIELDVALUE.(%s)\n",field,fieldvalue);
            if ( strcmp(field,"attachment") == 0 )
            {
                //getTransaction.5252503924337608312 {"sender":"18232225178877143084","fee":1,"amount":0,"timestamp":7755497,"referencedTransaction":"0","subtype":0,"attachment":{"message":"edfedada67000000"},"senderPublicKey":"35c0f29590d9d937fe8d5866351d592f423ecf506994ebae4a184c319de8140b","type":1,"deadline":720,"signature":"bd79f6b21e5c0c31001f2fc31e646515d5d5c81f0e7769325e11852a24ee2000f12b33798e373910af35e246816e80376a24c0dbd0147d1b4b8eb7f5b70ff03a","recipient":"18232225178877143084"}
                if ( strncmp(fieldvalue,"{\"asset\":\"",strlen("{\"asset\":\"")) == 0 )
                {
                    field = "asset";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"order\":\"",strlen("{\"order\":\"")) == 0 )
                {
                    field = "order";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"message\":\"",strlen("{\"message\":\"")) == 0 )
                {
                    field = "message";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"alias\":\"",strlen("{\"alias\":\"")) == 0 )
                {
                    field = "alias";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"price\":",strlen("{\"price\":")) == 0 )
                {
                    field = "price";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"description\":\"",strlen("{\"description\":\"")) == 0 )
                {
                    field = "description";
                    fieldvalue += strlen(field)+4;
                    printf("FIELD++.(%s) -> fieldvalue.(%s)\n",field,fieldvalue);
                }
            }
            if ( (j= normal_parse(&amount,fieldvalue,0)) < 0 )
            {
                printf("n.%d error processing field %s value %s j.%d (%s)\n",n,field,fieldvalue,j,tmpstr);
                return(0);
            }
            if ( fieldvalue[0] == '"' )
                token = fieldvalue+1;
            else token = fieldvalue;
            // printf("field.(%s) token.(%s) key.(%s)\n",field,token,keyname);
            (*processor)(field,token,keyname);
            valuestr = &fieldvalue[j];
            if ( valuestr[0] == '}' )
                valuestr++;
            if ( valuestr[0] != 0 )
                valuestr++;
            //printf("NEW VALUESTR(%s)\n",valuestr);
        }
        n++;
    }
    return(finalize_processor(processor));
}

void set_standard_AM(struct gateway_AM *ap,int funcid,char *nxtaddr,int timestamp)
{
    memset(ap,0,sizeof(*ap));
    ap->sig = GATEWAY_SIG;
    ap->funcid = funcid;
    ap->coinid = COINID;
    ap->gatewayid = 0;//GATEWAYID;
    ap->timestamp = timestamp;
    // ap->info = GATEWAYS[GATEWAYID];
    strcpy(ap->NXTaddr,nxtaddr);
}

char *submit_AM(struct gateway_AM *ap)
{
    int len,deadline = 1440;
    char hexbytes[4096],cmd[5120],*jsonstr,*reftxid = 0,*retstr = 0;
    len = (int)sizeof(*ap);
    if ( len > 1000 || len < 1 )
    {
        printf("issue_sendMessage illegal len %d\n",len);
        return(0);
    }
    // jl777: here is where the entire message should be signed;
    memset(hexbytes,0,sizeof(hexbytes));
    len = init_hexbytes(hexbytes,(void *)ap,len);
    sprintf(cmd,"%s=sendMessage&secretPhrase=%s&recipient=%s&message=%s&deadline=%u%s&fee=1",NXTSERVER,NXTACCTSECRET,NXTISSUERACCT,hexbytes,deadline,reftxid!=0?reftxid:"");
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        retstr = parse_NXTresults(0,"transaction","",results_processor,jsonstr,strlen(jsonstr));
        //printf("CMD.(%s) -> %s txid.%s\n",cmd,jsonstr,retstr);
        free(jsonstr);
    }
    return(retstr);
}

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

int is_gateway_related(struct gateway_AM *ap,char *sender)
{
    int i;
    if ( ap->sig == GATEWAY_SIG )
    {
        // good place to check for valid "website" token
        printf("sender.(%s) vs (%s)\n",sender,ap->NXTaddr);
        if ( strcmp(sender,ap->NXTaddr) == 0 )
            return(1);
        for (i=0; i<NUM_GATEWAYS; i++)
            if ( strcmp(sender,Gateway_NXTaddrs[i]) == 0 )
                return(1);
        if ( strcmp(sender,NXTISSUERACCT) == 0 )
            return(1);
    }
    return(0);
}

int process_NXTtransaction(char *nxt_txid)
{
    static int timestamp;
    struct gateway_AM AM;
    char cmd[4096],*jsonstr,*retstr;
    int gatewayid,n,tmp,flag = 0;
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,nxt_txid);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getTransaction.%s %s\n",nxt_txid,jsonstr);
        retstr = parse_NXTresults(0,"message","",results_processor,jsonstr,strlen(jsonstr));
        if ( retstr != 0 )
        {
            tmp = atoi(Timestamp);
            if ( tmp > timestamp )
                timestamp = tmp;
            
            n = (int)strlen(Message) / 2;
            memset(&AM,0,sizeof(AM));
            decode_hex((void *)&AM,n,Message);
            //for (int j=0; j<n; j++)
            //    printf("%02x",((char *)&AM)[j]&0xff);
            //printf("timestamp.%s %d\n",Timestamp,timestamp);
            if ( is_gateway_related(&AM,Sender) != 0 )
            {
                flag++;
                printf("GOT Gateway func.(%c)\n",AM.funcid);
                if ( AM.funcid == BIND_DEPOSIT_ADDRESS )
                {
                    printf("deposit address for %s is %s\n",AM.NXTaddr,AM.coinaddr);
                    strcpy(DEPOSITADDR,AM.coinaddr);
                }
            }
            free(retstr);
        }
        free(jsonstr);
    }
    return(timestamp);
}

char *issue_getState()
{
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=getState",NXTSERVER);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("\ngetState.(%s)\n\n",jsonstr);
        retstr = parse_NXTresults(0,"lastBlock","",results_processor,jsonstr,strlen(jsonstr));
        free(jsonstr);
    }
    return(retstr);
}

char *issue_getBlock(blockiterator arrayfunc,char *blockidstr)
{
    //int i,num;
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("\ngetBlock.%s %s\n",blockidstr,jsonstr);
        retstr = parse_NXTresults(arrayfunc,"numberOfTransactions","transactions",results_processor,jsonstr,strlen(jsonstr));
        free(jsonstr);
    }
    return(retstr);
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
