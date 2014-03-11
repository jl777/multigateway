
//  Created by jl777
//  MIT License
//

#ifndef NXTAPI_NXTparse_h
#define NXTAPI_NXTparse_h

typedef void *(*funcp)(char *field,char *arg,char *keyname);
typedef char *(*blockiterator)(char *blockidstr);

int Numinlist,Maxinlist;
char **List;

#include <memory.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdarg.h>


#define MAX_TOKEN_LEN 4096
struct MemoryStruct { char *memory; size_t size; };

char Sender[MAX_TOKEN_LEN],Block[MAX_TOKEN_LEN],Timestamp[MAX_TOKEN_LEN],Deadline[MAX_TOKEN_LEN],Account[MAX_TOKEN_LEN],Price[MAX_TOKEN_LEN],Balance[MAX_TOKEN_LEN],Asset[MAX_TOKEN_LEN],NumberOfUnlockedAccounts[MAX_TOKEN_LEN];
char Quantity[MAX_TOKEN_LEN],Asset[MAX_TOKEN_LEN],Recipient[MAX_TOKEN_LEN],Amount[MAX_TOKEN_LEN],Description[MAX_TOKEN_LEN];
char Fee[MAX_TOKEN_LEN],Confirmations[MAX_TOKEN_LEN],Signature[MAX_TOKEN_LEN],Bytes[MAX_TOKEN_LEN],Errorcode[MAX_TOKEN_LEN];
char Transaction[MAX_TOKEN_LEN],ReferencedTransaction[MAX_TOKEN_LEN],Subtype[MAX_TOKEN_LEN],Name[MAX_TOKEN_LEN],NextBlock[MAX_TOKEN_LEN];
char Message[MAX_TOKEN_LEN],SenderPublicKey[MAX_TOKEN_LEN],Type[MAX_TOKEN_LEN],Description[MAX_TOKEN_LEN],PreviousBlock[MAX_TOKEN_LEN];
char ShareAddress[MAX_TOKEN_LEN],Platform[MAX_TOKEN_LEN],Application[MAX_TOKEN_LEN],Weight[MAX_TOKEN_LEN],State[MAX_TOKEN_LEN],AnnouncedAddress[MAX_TOKEN_LEN],DownloadedVolume[MAX_TOKEN_LEN],Blacklisted[MAX_TOKEN_LEN],Version[MAX_TOKEN_LEN],UploadedVolume[MAX_TOKEN_LEN];


void reset_strings()
{
    Sender[0] = Block[0] = NumberOfUnlockedAccounts[0] = Timestamp[0] = Deadline[0] = Quantity[0] = Asset[0] = Description[0] = Account[0] = PreviousBlock[0] =
    Recipient[0] = Amount[0] = Fee[0] = Confirmations[0] = Signature[0] = Bytes[0] = Transaction[0] = Errorcode[0] = NextBlock[0] =
    ReferencedTransaction[0] = Message[0] = SenderPublicKey[0] = Subtype[0] = Type[0] = Name[0] = Description[0] = Price[0] = Balance[0] = Asset[0] = 0;
    ShareAddress[0] = Platform[0] = Application[0] = Weight[0] = State[0] = AnnouncedAddress[0] = DownloadedVolume[0] = Blacklisted[0] = Version[0] = UploadedVolume[0] = 0;
}

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    //printf("WriteMemoryCallback\n");
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

char *issue_curl(const char *postfields,...)
{
	//va_list p;
	//char *h;
    char *retstr;
    CURL *curl_handle;
    CURLcode res;
	//struct curl_slist *headerlist = NULL;

    // from http://curl.haxx.se/libcurl/c/getinmemory.html
    struct MemoryStruct chunk;
    chunk.memory = mymalloc(1);  // will be grown as needed by the realloc above
    chunk.size = 0;    // no data at this point
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYHOST,0);
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYPEER,0);
    curl_easy_setopt(curl_handle, CURLOPT_URL,postfields); // specify URL to get
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); // send all data to this function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk); // we pass our 'chunk' struct to the callback function
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // some servers don't like requests that are made without a user-agent field, so we provide one
 
	/*if ( postfields != NULL )
		curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,postfields);
    
	va_start(p,postfields);
	while( (h = va_arg (p, char *)) != NULL )
		headerlist = curl_slist_append(headerlist,h);
	va_end(p);
	if ( headerlist != NULL )
		curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,headerlist);*/

    res = curl_easy_perform(curl_handle);
    if ( res != CURLE_OK )
        fprintf(stderr, "curl_easy_perform() failed: %s (%s)\n",curl_easy_strerror(res),postfields);
    else
    {
        // printf("%lu bytes retrieved [%s]\n", (int64_t )chunk.size,chunk.memory);
    }
    curl_easy_cleanup(curl_handle);
    retstr = mymalloc(strlen(chunk.memory)+1);
    strcpy(retstr,chunk.memory);
    free(chunk.memory);
    return(retstr);
}

int64_t  stripstr(char *buf,int64_t  len)
{
    int i,j,c;
    for (i=j=0; i<len; i++)
    {
        c = buf[i];
        if ( c == '\\' )
            c = buf[i+1], i++;
        buf[j] = c;
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
    *amountp = 0;
    if ( strncmp(buf,"false",5) == 0 )
    {
        buf[5] = 0;
        return(5+1);
    }
    else if ( strncmp(buf,"true",4) == 0 )
    {
        buf[4] = 0;
        return(4+1);
    }
    else if ( strncmp(buf,"null",4) == 0 )
    {
        buf[4] = 0;
        return(4+1);
    }
    else if ( strncmp(buf,"\"\",",3) == 0 || strncmp(buf,"\"\"}",3) == 0 )
    {
        buf[0] = 0;
        return(2);
    }
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
    if ( buf[j] == '"' )
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
        if ( buf[j] == '"' )
            j++;
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
            if ( buf[j+1] == '"' )
                j++;
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
        if ( buf[j+1] == '"' )
            j++;
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

char *add_clones_toList(char *str)
{
    if ( Numinlist >= Maxinlist )
    {
        Maxinlist += 100;
        List = realloc(List,Maxinlist * sizeof(*List));
    }
    List[Numinlist++] = clonestr(str);
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
        else if ( strcmp("name",field) == 0 )
        {
            strcpy(Name,argstr);
            //printf("Name.(%s)\n",Name);
        }
        else if ( strcmp("description",field) == 0 )
        {
            if ( argstr[0] == 0 )
                strcpy(Description,"no description");
            else strcpy(Description,argstr);
        }
        else if ( strcmp("quantity",field) == 0 )
        {
            strcpy(Quantity,argstr);
            //printf("Quantity.(%s)\n",Quantity);
        }
        else if ( strcmp("account",field) == 0 || strcmp("accountId",field) == 0 )
        {
            strcpy(Account,argstr);
            //printf("Account.(%s)\n",Account);
        }
        else if ( strcmp("fee",field) == 0 )
            strcpy(Fee,argstr);
        else if ( strcmp("numberOfUnlockedAccounts",field) == 0 )
            strcpy(NumberOfUnlockedAccounts,argstr);
        else if ( strcmp("shareAddress",field) == 0 )
            strcpy(ShareAddress,argstr);
        else if ( strcmp("platform",field) == 0 )
            strcpy(Platform,argstr);
        else if ( strcmp("application",field) == 0 )
            strcpy(Application,argstr);
        else if ( strcmp("weight",field) == 0 )
            strcpy(Weight,argstr);
        else if ( strcmp("state",field) == 0 )
            strcpy(State,argstr);
        else if ( strcmp("announcedAddress",field) == 0 )
            strcpy(AnnouncedAddress,argstr);
        else if ( strcmp("downloadedVolume",field) == 0 )
            strcpy(DownloadedVolume,argstr);
        else if ( strcmp("blacklisted",field) == 0 )
            strcpy(Blacklisted,argstr);
        else if ( strcmp("version",field) == 0 )
            strcpy(Version,argstr);
        else if ( strcmp("uploadedVolume",field) == 0 )
            strcpy(UploadedVolume,argstr);
        else if ( strcmp("balance",field) == 0 )
            strcpy(Balance,argstr);
        else if ( strcmp("asset",field) == 0 )
            strcpy(Asset,argstr);
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
        else if ( strcmp("errorCode",field) == 0 )
            strcpy(Errorcode,argstr);
        else if ( strcmp("previousBlock",field) == 0 )
            strcpy(PreviousBlock,argstr);
        else if ( strcmp("nextBlock",field) == 0 )
            strcpy(NextBlock,argstr);
        else if ( strcmp("price",field) == 0 )
            strcpy(Price,argstr);
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
        //printf("resultstr.(%s)\n",resultstr);
        n = (int)strlen(resultstr);
        if ( n > 0 )
        {
            token = mymalloc(n+1);
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
    static char *tmpstr;
    int j,n;
    double amount;
    char *token,*valuestr,*field,*fieldvalue,*blockidstr;
    if ( tmpstr == 0 )
        tmpstr = malloc(1000000);
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
            printf("field error.%d error parsing results(%s) [%s] [%s] (%s)\n",n,results,fieldvalue,field,tmpstr);
            return(0);
        }
        if ( fieldvalue[0] == ':' )
            fieldvalue++;
        if ( fieldvalue[0] == '[' )
        {
            fieldvalue++;
            if ( arrayfield != 0 && strcmp(arrayfield,field) != 0 )
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
                    //printf("(%s.%d %s)\n",field,n,blockidstr);
                    (*iterator)(blockidstr);
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
                else if ( strncmp(fieldvalue,"{\"name\":\"",strlen("{\"name\":\"")) == 0 )
                {
                    field = "name";
                    fieldvalue += strlen(field)+4;
                }
                else if ( strncmp(fieldvalue,"{\"quantity\":\"",strlen("{\"quantity\":\"")) == 0 )
                {
                    field = "quantity";
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
                    //printf("FIELD++.(%s) -> fieldvalue.(%s)\n",field,fieldvalue);
                }
            }
            if ( (j= normal_parse(&amount,fieldvalue,0)) < 0 )
            {
                printf("n.%d error processing field %s value.(%s) j.%d (%s)\n",n,field,fieldvalue,j,tmpstr);
                return(0);
            }
            if ( fieldvalue[0] == '"' )
                token = fieldvalue+1;
            else token = fieldvalue;
            //printf("field.(%s) token.(%s) key.(%s) -> j.%d (%s)\n",field,token,keyname,j,fieldvalue);
            valuestr = &fieldvalue[j];
            (*processor)(field,token,keyname);
            if ( valuestr[0] == '}' )
                valuestr++;
            if ( valuestr[0] != 0 && valuestr[0] != '"' )
                valuestr++;
            //printf("NEW VALUESTR(%s)\n",valuestr);
        }
        n++;
    }
    //printf("finalize\n");
    return(finalize_processor(processor));
}


#endif
