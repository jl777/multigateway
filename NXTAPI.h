//
//  Created by jl777
//  MIT License
//

#ifndef NXTAPI_NXTAPI_h
#define NXTAPI_NXTAPI_h


char *issue_transferAsset(char *secret,char *recipient,char *asset,int64_t quantity,int fee,int deadline,char *reftxid)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=transferAsset&secretPhrase=%s&recipient=%s&asset=%s&quantity=%ld&fee=%d&deadline=%d&%s",NXTSERVER,secret,recipient,asset,(long)quantity,fee,deadline,reftxid!=0?reftxid:"");
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("transferAsset.%s <-= %.2f of %s (%s)\n",recipient,(double)quantity,asset,jsonstr);
    }
    return(jsonstr);
}

char **issue_getAssetIds()
{
    int i;
    char cmd[4096],*jsonstr,*retstr,**assetids = 0;
    sprintf(cmd,"%s=getAssetIds",NXTSERVER);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAssetIds.(%s)\n",jsonstr);
        Numinlist = 0;
        retstr = parse_NXTresults(add_clones_toList,"assetIds","assetIds",results_processor,jsonstr,strlen(jsonstr));
        if ( retstr != 0 )
            myfree(retstr,"27");
        printf("getAssetIds.ret.(%s) Numinlist.%d\n",retstr,Numinlist);
        if ( Numinlist > 0 )
        {
            assetids = mymalloc(sizeof(*assetids) * (Numinlist+1));
            for (i=0; i<Numinlist; i++)
            {
                assetids[i] = List[i];
                List[i] = 0;
                printf("%s ",assetids[i]);
            }
            assetids[i] = 0;
        }
    }
    return(assetids);
}

char *issue_getAsset(char *assetidstr)
{
    char cmd[4096],*jsonstr,*retstr;
    sprintf(cmd,"%s=getAsset&asset=%s",NXTSERVER,assetidstr);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getAsset.%s %s\n",assetidstr,jsonstr);
        retstr = parse_NXTresults(0,"","",results_processor,jsonstr,strlen(jsonstr));
        if ( retstr != 0 )
            myfree(retstr,"28");
        //myfree(jsonstr,"29");
        return(jsonstr);
    }
    return(0);
}

char *issue_getAccountBlockIds(char *acctid,int timestamp)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getAccountBlockIds&account=%s&timestamp=%d",NXTSERVER,acctid,timestamp);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAccountBlockIds.%s %s\n",acctid,jsonstr);
        myfree(jsonstr,"30");
    }
    return(0);
}

char *issue_getMyInfo()
{
    char cmd[4096],*jsonstr=0,*retstr = 0;
    sprintf(cmd,"%s=getMyInfo",NXTSERVER);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getMyInfo -> (%s)\n",jsonstr);
        retstr = parse_NXTresults(0,"address","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"30");
    }
    return(retstr);
}

char *issue_getAccount(char *acctid)
{
    char cmd[4096],*jsonstr=0;
    sprintf(cmd,"%s=getAccount&account=%s",NXTSERVER,acctid);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getAccount.%s -> (%s)\n",acctid,jsonstr);
        //retstr = parse_NXTresults(0,"","assetBalances",results_processor,jsonstr,strlen(jsonstr));
        // if ( retstr != 0 )
        //     myfree(retstr,"88");
        //myfree(jsonstr,"30");
    }
    return(jsonstr);
}

char *issue_getAccountPublicKey(char *acctid)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getAccountPublicKey&account=%s",NXTSERVER,acctid);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAccountPublicKey.%s %s\n",acctid,jsonstr);
        myfree(jsonstr,"31");
    }
    return(0);
}

char *issue_getAccountTransactionIds(blockiterator arrayfunc,char *acctid,int timestamp)
{
    char cmd[4096],*jsonstr,*retstr=0;
    sprintf(cmd,"%s=getAccountTransactionIds&account=%s&timestamp=%d",NXTSERVER,acctid,timestamp);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getAccountTransactionIds.%s %s\n",acctid,jsonstr);
        if ( arrayfunc != 0 )
            retstr = parse_NXTresults(arrayfunc,"sender","transactionIds",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"32");
    }
    return(retstr);
}

char *issue_getAccountId(char *password)
{
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=getAccountId&secretPhrase=%s",NXTSERVER,password);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAccountId.(%s) -> %s\n",password,jsonstr);
        retstr = parse_NXTresults(0,"accountId","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"33");
    }
    return(retstr);
}

char *issue_getBalance(char *acctid)
{
    char cmd[4096],*jsonstr,*retstr;
    sprintf(cmd,"%s=getBalance&account=%s",NXTSERVER,acctid);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getBalance.%s %s\n",acctid,jsonstr);
        retstr = parse_NXTresults(0,"balance","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"33");
        return(retstr);
    }
    return(0);
}

char *issue_getGuaranteedBalance(char *acctid,int numconfs)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getGuaranteedBalance&account=%s&numberOfConfirmations=%d",NXTSERVER,acctid,numconfs);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getGuaranteedBalance.%s %s\n",acctid,jsonstr);
        myfree(jsonstr,"34");
    }
    return(0);
}

char *issue_getAliasId(char *alias)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getAliasId&alias=%s",NXTSERVER,alias);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAliasId.%s = %s\n",alias,jsonstr);
        myfree(jsonstr,"35");
    }
    return(0);
}


char *issue_getTransaction(char *txidstr)
{
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,txidstr);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getTransaction.%s %s\n",txidstr,jsonstr);
        retstr = parse_NXTresults(0,"sender","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"40");
    } else printf("error getting txid.%s\n",txidstr);
    return(retstr);
}

char *issue_getAliasIds(char *acctid,int timestamp)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getAliasIds&account=%s&timestamp=%d",NXTSERVER,acctid,timestamp);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAliasIds.%s %s\n",acctid,jsonstr);
        myfree(jsonstr,"36");
    }
    return(0);
}

char *issue_getAliasURI(char *alias)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getAliasURI&alias=%s",NXTSERVER,alias);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("getAliasURI.%s = %s\n",alias,jsonstr);
        myfree(jsonstr,"37");
    }
    return(0);
}

char *issue_listAccountAliases(char *acctid)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=listAccountAliases&account=%s",NXTSERVER,acctid);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("listAccountAliases.%s %s\n",acctid,jsonstr);
        myfree(jsonstr,"38");
    }
    return(0);
}

char *issue_getTransactionBytes(char *txidstr)
{
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getTransactionBytes&transaction=%s",NXTSERVER,txidstr);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        //printf("getTransactionBytes.%s %s\n",txidstr,jsonstr);
        myfree(jsonstr,"39");
    }
    return(0);
}

char *issue_startForging()
{
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=startForging&secretPhrase=%s",NXTSERVER,NXTACCTSECRET);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("\nissue_startForging.(%s)\n\n",jsonstr);
        myfree(jsonstr,"98");
    }
    return(retstr);
}

char *issue_getState()
{
    static int counter;
    char cmd[4096],*jsonstr,*retstr = 0;
    sprintf(cmd,"%s=getState",NXTSERVER);
    //printf("cmd (%s)\n",cmd);
    jsonstr = issue_curl(cmd);//issue_curl("nxt?requestType=getState");
    if ( jsonstr != 0 )
    {
        if ( counter++ == 0 )
            printf("\ngetState.(%s)\n\n",jsonstr);
        retstr = parse_NXTresults(0,"lastBlock","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"41");
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
        myfree(jsonstr,"42");
    }
    return(retstr);
}

char *issue_sendMessage(char *secret,char *recipient,unsigned char *message,long len,int deadline,char *reftxid)
{
    char hexbytes[4096],cmd[5120],*jsonstr,*retstr = 0;
    if ( len > 1000 || len < 1 )
    {
        printf("issue_sendMessage illegal len %ld\n",len);
        return(0);
    }
    memset(hexbytes,0,sizeof(hexbytes));
    len = init_hexbytes(hexbytes,message,len);
    sprintf(cmd,"%s=sendMessage&secretPhrase=%s&recipient=%s&message=%s&deadline=%u%s&fee=1",NXTSERVER,secret,recipient,hexbytes,deadline,reftxid!=0?reftxid:"");
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        retstr = parse_NXTresults(0,"transaction","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"44");
    }
    return(retstr);
}

int test_NXTAPI(char *acctid)
{
    int deadline;
    char *blockidstr;
    unsigned char *teststr;
    char *retstr;
    printf("test_NXTAPI(acctid: %s %s)\n",acctid,acctid);
    if ( 0 )
    {
        issue_getAccountBlockIds(acctid,0);
        issue_getAccountPublicKey(acctid);
        issue_getAccountTransactionIds(0,acctid,0);
        issue_getBalance(acctid);
        issue_getGuaranteedBalance(acctid,15);
        issue_getAliasId("NXTcommunityfund");
        //issue_getAliasIds(acctid,0);
        issue_getAliasURI("NXTcommunityfund");
        issue_listAccountAliases(acctid);
        issue_getTransactionBytes(NXTACCT);
        issue_getTransaction("11532828229876056821");
    }
    blockidstr = issue_getState();
    if ( blockidstr != 0 )
    {
        issue_getBlock(issue_getTransaction,blockidstr);
        myfree(blockidstr,"45");
    }
    teststr = (unsigned char *)"this is a test message";
    deadline = 720;
    retstr = issue_sendMessage(NXTACCTSECRET,NXTACCT,teststr,strlen((char *)teststr)+1,deadline,0);
    if ( 0 && retstr != 0 )
    {
        printf("AM txid=[%s]\n",retstr);
        myfree(retstr,"46");
    }
    return(0);
}

void set_standard_AM(struct gateway_AM *ap,int funcid,char *nxtaddr,int timestamp)
{
    memset(ap,0,sizeof(*ap));
    ap->sig = GATEWAY_SIG;
    ap->funcid = funcid;
    ap->coinid = COINID;
    ap->gatewayid = GATEWAYID;
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
    // jl777: here is where the entire message could be signed;
    memset(hexbytes,0,sizeof(hexbytes));
    len = init_hexbytes(hexbytes,(void *)ap,len);
    sprintf(cmd,"%s=sendMessage&secretPhrase=%s&recipient=%s&message=%s&deadline=%u%s&fee=1",NXTSERVER,NXTACCTSECRET,NXTISSUERACCT,hexbytes,deadline,reftxid!=0?reftxid:"");
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        printf("CMD.(%s) -> %s txid.%s\n",cmd,jsonstr,retstr);
        retstr = parse_NXTresults(0,"transaction","",results_processor,jsonstr,strlen(jsonstr));
        myfree(jsonstr,"51");
    }
    return(retstr);
}


#endif
