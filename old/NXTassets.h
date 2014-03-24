//  Created by jl777
//  MIT License
//


#ifndef gateway_NXTassets_h
#define gateway_NXTassets_h

struct strings ASSETNAMES,ASSETIDS,ASSETCHANGES;
struct active_NXTacct **NXTaccts; int Numactive,Maxactive;

char *tradestr = "{\"trades\":[{\"timestamp\":8059372,\"price\":150,\"bidOrderId\":\"16384447982904651837\",\"askOrderId\":\"9791922072641732772\",\"quantity\":111},{\"timestamp\":8918487,\"price\":150,\"bidOrderId\":\"10793175780712350450\",\"askOrderId\":\"17320883098321475085\",\"quantity\":100},{\"timestamp\":8988777,\"price\":1000,\"bidOrderId\":\"484563926704674434\",\"askOrderId\":\"5311349685613017048\",\"quantity\":10}]}";

//{"trades":[{"timestamp":9113330,"price":100,"bidOrderId":"3084760732202099893","askOrderId":"4350660502749413964","quantity":500}]}

int parse_trades(char *list,int assetid,char *assetidstr,char *assetname)
{
    struct NXT_trade T;
    char txidstr[100];
    int len,n,i,j,k,tmp,numtrades = 0;
    char *argstr,*blockstr,*valuestr,*fieldvalue,*field;
    if ( list != 0 )
    {
        len = (int)stripstr(list,strlen(list));
        if ( strncmp(list,"{\"trades\":[]}",strlen("{\"trades\":[]}")) == 0 )
            return(0);
        n = 10; // skip {"trades":
        //printf("%s\n",list);
        //printf("n.%d len-2=%d, (%c) (%c)\n",n,len-2,list[n],list[len-2]);
        if ( list[n] == '[' && list[len-2] == ']' && n < len-3 )
        {
            //printf("Inside\n");
            argstr = mymalloc(len+1);
            blockstr = mymalloc(len+1);
            j = n+1;
            while ( 1 )
            {
                memset(&T,0,sizeof(T));
                i = j;
                valuestr = &list[j];
                for (; j<len-1; j++)
                    if ( list[j] == '}' )
                        break;
                if ( j < len-1 )
                {
                    memcpy(blockstr,valuestr,j-i+1);
                    blockstr[j-i+1] = 0;
                    if ( blockstr[0] == '{' && blockstr[j-i] == '}' )
                    {
                        blockstr[j-i] = 0;
                        fieldvalue = blockstr+1;
                        printf("(%s)\n",fieldvalue);
                        while ( *fieldvalue != 0 )
                        {
                            fieldvalue = decode_json(&field,fieldvalue);
                            if ( fieldvalue == 0 || field == 0 || *fieldvalue != ':' )
                            {
                                printf("field error.%d error parsing results(%s) [%s] [%s]\n",n,list,fieldvalue,field);
                                //myfree(list,"0");
                                myfree(blockstr,"1");
                                myfree(argstr,"2");
                                return(0);//(ptrlist);
                            }
                            fieldvalue++;
                            if ( fieldvalue[0] == '[' )
                            {
                                for (k=0; k<j-i; k++)
                                {
                                    if ( fieldvalue[k] == ']' )
                                        break;
                                }
                                memcpy(argstr,fieldvalue+1,k-1);
                                argstr[k-1] = 0;
                                k += 2;
                            }
                            else
                            {
                                tmp = (fieldvalue[0] == '"') ? 1 : 0;
                                k = 1 + tmp + copy_str(argstr,&fieldvalue[tmp],j-i);
                            }
                            fieldvalue += k;
                            if ( *fieldvalue == ',' )
                                fieldvalue++;
                            //printf("numtrades.%d field.(%s) arg.(%s) -> k.%d\n",numtrades,field,argstr,k);
                            if ( strcmp(field,"timestamp") == 0 )
                                T.timestamp = atoi(argstr);
                            else if ( strcmp(field,"price") == 0 )
                                T.price = ((int64_t)atoi(argstr) * SATOSHIDEN) / 100;
                            else if ( strcmp(field,"quantity") == 0 )
                                T.quantity = ((int64_t)atoi(argstr) * SATOSHIDEN);
                            else if ( strcmp(field,"bidOrderId") == 0 )
                                T.buyer = clonestr(argstr);
                            else if ( strcmp(field,"askOrderId") == 0 )
                                T.seller = clonestr(argstr);
                        }
                        T.assetid = assetid;
                        sprintf(txidstr,"%s.%s",T.buyer,T.seller);
                        T.txid = clonestr(txidstr);
                        if ( T.timestamp <= 0 || T.price <= 0 || T.quantity <= 0 || T.buyer == 0 || T.seller == 0 )
                            printf("INVALID trade???\n");
                        else
                        {
                            char *retstr;
                            retstr = issue_getTransaction(T.buyer);
                            myfree(T.buyer,"69");
                            T.buyer = clonestr(Sender);
                            if ( retstr != 0 )
                                myfree(retstr,"84");
                            retstr = issue_getTransaction(T.seller);
                            myfree(T.seller,"68");
                            T.seller = clonestr(Sender);
                            if ( retstr != 0 )
                                myfree(retstr,"85");
                            update_asset_balances(assetname,T.txid,assetidstr,T.buyer,T.seller,T.quantity);
                            printf("%s %.8f %.8f timestamp.%d | buyer.%s seller.%s\n",T.txid,(double)T.quantity/SATOSHIDEN,(double)T.price/SATOSHIDEN,T.timestamp,T.buyer,T.seller);
                        }
                        if ( T.txid != 0 )
                            myfree(T.txid,"78");
                        if ( T.buyer != 0 )
                            myfree(T.buyer,"79");
                        if ( T.seller != 0 )
                            myfree(T.seller,"80");
                        numtrades++;
                    }
                    j += 2;
                } else break;
            }
            myfree(blockstr,"5");
            myfree(argstr,"6");
        }
        //printf("returning %ld ptrs\n",(long)numptrs);
        //myfree(list,"4");
    }
    return(numtrades);
}

int issue_getTrades(int assetid,char *assetidstr,char *assetname,int starti,int endi)
{
    int numtrades = 0;
    char cmd[4096],*jsonstr,*tmpstr;
    if ( endi == 0 )
        endi = (1 << 30);
    sprintf(cmd,"%s=getTrades&asset=%s&firstIndex=%d&lastIndex=%d",NXTSERVER,assetidstr,starti,endi);
    jsonstr = issue_curl(cmd);
    if ( jsonstr != 0 )
    {
        if ( strncmp(jsonstr,"{\"error\":",strlen("{\"error\":")) != 0 && strncmp(jsonstr,"{\"trades\":[]}",strlen("{\"trades\":[]}")) != 0 )
        {
            printf("(%s) -> %s\n",cmd,jsonstr);
            tmpstr = clonestr(jsonstr);
            myfree(jsonstr,"82");
            numtrades = parse_trades(tmpstr,assetid,assetidstr,assetname);
            myfree(tmpstr,"83");
        } else myfree(jsonstr,"82");
    }
    return(numtrades);
}

int validate_nxtaddr(char *nxtaddr)
{
    // make sure it has publickey
    int n = (int)strlen(nxtaddr);
    while ( n > 10 && (nxtaddr[n-1] == '\n' || nxtaddr[n-1] == '\r') )
        n--;
    if ( n < 1 )
        return(-1);
    return(0);
}

int validate_coinaddr(char *coinaddr)
{
    // make sure it has valid checksum
    int n = (int)strlen(coinaddr);
    while ( n > 10 && (coinaddr[n-1] == '\n' || coinaddr[n-1] == '\r') )
        n--;
    return(0);
}

struct active_NXTacct *search_NXTaccts(char *nxtaddr)
{
    int i;
    for (i=0; i<Numactive; i++)
    {
        if ( strcmp(nxtaddr,NXTaccts[i]->NXTaddr) == 0 )
            return(NXTaccts[i]);
    }
    return(0);
}

int add_NXTacct(char *nxtaddr)
{
    if ( Numactive >= Maxactive )
    {
        Maxactive += 100;
        NXTaccts = realloc(NXTaccts,sizeof(*NXTaccts) * Maxactive);
    }
    printf("ADD NXTacct.%d (%s)\n",Numactive,nxtaddr);
    NXTaccts[Numactive] = malloc(sizeof(*NXTaccts[Numactive]));
    memset(NXTaccts[Numactive],0,sizeof(*NXTaccts[Numactive]));
    strcpy(NXTaccts[Numactive]->NXTaddr,strip_tohexcodes(nxtaddr));
    Numactive++;
    return(Numactive);
}

struct active_NXTacct *get_active_NXTacct(char *nxtaddr)
{
    int i;
    struct active_NXTacct *active;
    if ( nxtaddr[0] == 0 )
        return(0);
    for (i=0; nxtaddr[i]; i++)
        if ( nxtaddr[i] < '0' || nxtaddr[i] > '9' )
            return(0);
    if ( validate_nxtaddr(nxtaddr) == 0 )
    {
        if ( (active= search_NXTaccts(nxtaddr)) == 0 )
            add_NXTacct(nxtaddr);
        if ( (active= search_NXTaccts(nxtaddr)) == 0 )
        {
            printf("FATAL ERROR: couldnt add NXTaddr!\n");
            while ( 1 ) sleep(1);
        }
        return(active);
    }
    else
    {
        printf("illegal nxt acct (%s) ignored\n",nxtaddr);
        return(0);
    }
}

char *teststr2 = "{\"publicKey\":\"283f18748e80e38d83edcff6f623eac829a3494deb1c662b66e7b7ee488cd072\",\"balance\":405100,\"assetBalances\":[{\"balance\":5,\"asset\":\"11223965817159725098\"},{\"balance\":19,\"asset\":\"7761388364129412234\"}],\"effectiveBalance\":405100}";

int verify_account_assets(int dispflag,struct active_NXTacct *active)
{
    double amount;
    int64_t balance,*balances;
    int assetid,i,j,errors,nonz,missing;
    char *jsonstr,*str,*argstr,*field,assetidstr[128];
    if ( strcmp(active->NXTaddr,GENESISACCT) == 0 )
        return(0);
    balances = mymalloc(sizeof(*balances) * (ASSETIDS.num+1));
    memset(balances,0,sizeof(*balances) * (ASSETIDS.num+1));
    jsonstr = issue_getAccount(active->NXTaddr);
    errors = missing = nonz = 0;
    if ( jsonstr != 0 )
    {
        str = strstr(jsonstr,"\"assetBalances\":[");
        if ( str != 0 )
        {
            str += strlen("\"assetBalances\":[");
            while ( *str != 0 && *str == '{' )
            {
                str++;
                balance = 0;
                assetidstr[0] = 0;
                while ( *str != '}' )
                {
                    argstr = decode_json(&field,str);
                    if ( argstr != 0 && field != 0 )
                    {
                        if ( argstr[0] == ':' )
                            argstr++;
                        if ( (j= normal_parse(&amount,argstr,0)) < 0 )
                        {
                            printf("error verify_account_assets field %s value.(%s) j.%d\n",field,argstr,j);
                            return(-1);
                        }
                        str = &argstr[j];
                        if ( *argstr == '"' )
                            argstr++;
                        //printf("(%s %s) -> (%s) ",field,argstr,str);
                        if ( strcmp(field,"balance") == 0 )
                            balance = (int64_t)atoi(argstr) * SATOSHIDEN;
                        else if ( strcmp(field,"asset") == 0 )
                        {
                            for (i=0; i<127; i++)
                            {
                                if ( argstr[i] < '0' || argstr[i] > '9' )
                                    break;
                                assetidstr[i] = argstr[i];
                            }
                            assetidstr[i] = 0;
                        }
                        if ( *str == ',' )
                            str++;
                    } else break;
                }
                assetid = find_string(&ASSETIDS,assetidstr);
                if ( assetid >= 0 && balance != 0 )
                {
                    if ( assetid >= ASSETIDS.num )
                        printf("OVERFLOW ");
                    //printf("[assetid.%d %s %s %.8f] ",assetid,ASSETNAMES.list[ASSETIDS.args[assetid]],assetidstr,(double)balance/SATOSHIDEN);
                    balances[assetid] = balance;
                    nonz++;
                }
                str++;
                if ( *str == ',' )
                    str++;
            }
            //printf("parsed\n");
        }
        myfree(jsonstr,"101");
    }
    for (assetid=0; assetid<active->numassets; assetid++)
    {
        if ( active->balances[assetid] != 0 )
            nonz++;//, printf("%d.(%s %.8f) ",assetid,ASSETIDS.list[assetid],(double)active->balances[assetid]/SATOSHIDEN);
    }
    for (assetid=0; assetid<ASSETIDS.num; assetid++)
    {
        if ( assetid < active->numassets )
        {
            if ( active->balances[assetid] != balances[assetid] )
                printf("(MISMATCH.%d %.8f vs %.8f) ",assetid,(double)active->balances[assetid]/SATOSHIDEN,(double)balances[assetid]/SATOSHIDEN), errors++;
        }
        else if ( dispflag != 0 && balances[assetid] != 0 )
            printf("MISSING.%d ",assetid), missing++;
    }
    if ( dispflag != 0 && nonz+errors+missing != 0 )
        printf("acct.%s assets nonz.%d errors.%d missing.%d\n",active->NXTaddr,nonz,errors,missing);
    myfree(balances,"111");
    active->asseterrors = errors;
    active->assetmissing = missing;
    active->assetnonz = nonz;
    return(errors+missing);
}

int verify_accounts_assets(int *nonzp,int *totalp)
{
    int i,nonz,total,errs = 0;
    nonz = total = 0;
    for (i=0; i<Numactive; i++)
    {
        if ( verify_account_assets(0,NXTaccts[i]) > 0 )
            errs += verify_account_assets(1,NXTaccts[i]);
        if ( NXTaccts[i]->assetnonz != 0 )
            nonz++, total += NXTaccts[i]->assetnonz;
    }
    *nonzp = nonz;
    *totalp = total;
    return(errs);
}

void verify_assetspace(struct active_NXTacct *active,int num)
{
    int assetid;
    if ( num > active->numassets )
    {
        active->balances = realloc(active->balances,ASSETNAMES.num * sizeof(*active->balances));
        for (assetid=active->numassets; assetid<ASSETNAMES.num; assetid++)
            active->balances[assetid] = 0;
        active->numassets = num;
    }
}

void set_asset_name(char *name,char *assetidstr)
{
    int assetid;
    assetid = find_string(&ASSETIDS,assetidstr);
    if ( assetid < 0 )
    {
        printf("SERIOUS ERROR: set_asset_name couldnt find (%s)\n",assetidstr);
        if ( BLOCK_ON_SERIOUS != 0 )
            while ( 1 ) sleep(1);
    }
    if ( ASSETIDS.args[assetid] >= 0 && ASSETIDS.args[assetid] < ASSETIDS.num )
        strcpy(name,ASSETNAMES.list[ASSETIDS.args[assetid]]);
    else
    {
        printf("SERIOUS ERROR: set_asset_name assetid.%d (%s) has no argptr\n",assetid,assetidstr);
        if ( BLOCK_ON_SERIOUS != 0 )
            while ( 1 ) sleep(1);
    }
}

int update_assetname(char *assetname)
{
    int nameid = -1;;
    nameid = find_string(&ASSETNAMES,assetname);
    if ( nameid < 0 )
    {
        add_string(&ASSETNAMES,assetname,-1,0,0,0);
        printf("new ASSETNAME.(%s)\n",assetname);
    }
    nameid = find_string(&ASSETNAMES,assetname);
    if ( nameid < 0 )
    {
        printf("FATAL ERROR: failed adding assetname.(%s)\n",assetname);
        while ( 1 ) sleep(1);
    }
    return(nameid);
}

void update_asset_balances(char *assetname,char *change_txid,char *assetidstr,char *recipientstr,char *senderstr,int64_t satoshis)
{
    // assetname and assetidstr are mutually exclusive, assetname only for issuance
    int assetid,nameid;
    struct active_NXTacct *receiver,*sender;
    //printf("update_asset_balances %s %s %s %s\n",assetname,assetidstr,recipientstr,senderstr);
    receiver = get_active_NXTacct(recipientstr);
    sender = get_active_NXTacct(senderstr);
    nameid = find_string(&ASSETNAMES,assetname);
    assetid = find_string(&ASSETIDS,assetidstr);
    if ( receiver == 0 || sender == 0 || assetid < 0 || nameid < 0 )
    {
        printf("couldnt find %s or %s or ids problem (%s) %d | (%s) %d\n",recipientstr,senderstr,assetidstr,assetid,assetname,nameid);
        return;
    }
    verify_assetspace(receiver,ASSETIDS.num);
    verify_assetspace(sender,ASSETIDS.num);
    //printf("(%s) assetid.%d %s %s -> %s, %.8f\n",assetname,assetid,assetidstr,senderstr,recipientstr,(double)satoshis/SATOSHIDEN);
    if ( assetid >= 0 )
    {
        if ( add_string(&ASSETCHANGES,change_txid,assetid,satoshis,receiver,sender) != 0 )
        {
            if ( strcmp(GENESISACCT,senderstr) != 0 && sender->balances[assetid] < satoshis )
            {
                printf("ILLEGAL transfer %s assetid.%d to %s | %s.(%.8f) < xfer.%8f????\n",ASSETIDS.list[assetid],assetid,recipientstr,senderstr,(double)sender->balances[assetid]/SATOSHIDEN,(double)satoshis/SATOSHIDEN);
                //if ( BLOCK_ON_SERIOUS != 0 )
                //    while ( 1 ) sleep(1);
            }
            //else
            {
                receiver->balances[assetid] += satoshis;
                sender->balances[assetid] -= satoshis;
                printf(">>>>>>>>>>>>>>>>> transfer %s assetid.%d %.8f from %s.(%.8f) to %s.(%.8f)\n",ASSETIDS.list[assetid],assetid,(double)satoshis/SATOSHIDEN,senderstr,(double)sender->balances[assetid]/SATOSHIDEN,recipientstr,(double)receiver->balances[assetid]/SATOSHIDEN);
            }
        }
    }
    else
    {
        printf("SERIOUS ERROR, cant find (%s)\n",assetidstr);
        if ( BLOCK_ON_SERIOUS != 0 )
            while ( 1 ) sleep(1);
    }
}

int update_assetid(char *assetidstr)
{
    int64_t quantity;
    int assetid,nameid,flag;
    char *issuer,*descr,*retstr,name[512];
    nameid = -1;
    if ( (retstr= issue_getAsset(assetidstr)) != 0 )
    {
        strcpy(name,Name);
        if ( name[0] != 0 )
            nameid = update_assetname(name);
        quantity = atoi(Quantity);
        if ( Description[0] == 0 )
            descr = clonestr("<no description>");
        else descr = clonestr(Description);
        issuer = clonestr(Account);
        flag = 0;
        assetid = find_string(&ASSETIDS,assetidstr);
        if ( quantity > 0 && nameid >= 0 && issuer[0] != 0 )
        {
            //int add_string(struct strings *ptrs,char *str,int arg,int64_t arg2,void *argptr,void *argptr2)
            if ( assetid < 0 && add_string(&ASSETIDS,assetidstr,nameid,0,issuer,descr) > 0 )
            {
                flag++;
                assetid = find_string(&ASSETIDS,assetidstr);
                printf("NEW ASSETID.(%s) assetid.%d for nameid.%d %s issuer.%s\n",assetidstr,assetid,nameid,name,issuer);
                update_asset_balances(name,assetidstr,assetidstr,issuer,clonestr(GENESISACCT),quantity*SATOSHIDEN);
                if ( ASSETNAMES.args[nameid] >= 0 || ASSETNAMES.argptrs[nameid] != 0 || ASSETNAMES.argptrs2[nameid] != 0 || ASSETNAMES.arg2[nameid] != 0 )
                {
                    if ( ASSETNAMES.args[nameid] != assetid || ASSETNAMES.arg2[nameid] != quantity ||
                        strcmp(ASSETNAMES.argptrs[nameid],issuer) != 0 || strcmp(ASSETNAMES.argptrs2[nameid],descr) != 0 )
                    {
                        printf("UNEXPECTED MISMATCH %.0f %.0f %s %s | %.0f %.0f %s %s\n",(double)ASSETNAMES.args[nameid],(double)ASSETNAMES.arg2[nameid],(char *)ASSETNAMES.argptrs[nameid],(char *)ASSETNAMES.argptrs2[nameid],
                               (double)assetid,(double)quantity,issuer,descr);
                    }
                }
                else
                {
                    ASSETNAMES.args[nameid] = assetid;
                    ASSETNAMES.arg2[nameid] = quantity;
                    ASSETNAMES.argptrs[nameid] = issuer;
                    ASSETNAMES.argptrs2[nameid] = descr;
                    printf("nameid.%d set.(%s) %.0f %.0f %s %s\n",nameid,ASSETNAMES.list[nameid],(double)ASSETNAMES.args[nameid],(double)ASSETNAMES.arg2[nameid],(char *)ASSETNAMES.argptrs[nameid],(char *)ASSETNAMES.argptrs2[nameid]);
                }
            }
            //else printf("quantity %.0f nameid.%d Account.(%s)\n",(double)quantity,nameid,issuer);
        }
        else
        {
            printf("UNEXPECTED asset mismatch assetid.%d quantity %.8f Name.%s Account.%s\n",assetid,(double)quantity/SATOSHIDEN,Name,Account);
            if ( BLOCK_ON_SERIOUS != 0 ) while ( 1 ) sleep(1);
        }
        if ( flag == 0 )
            myfree(descr,"60"),myfree(issuer,"61");
        myfree(retstr,"93");
    }
    return(nameid);
}

char *update_asset_names(char *assetname)
{
    int i,nameid;
    char **assetids,*assetidstr = 0;
    assetids = issue_getAssetIds();
    for (i=0; assetids[i]!=0; i++)
    {
        printf("i.%d of %d %p\n",i,Numinlist,assetids[i]);
        nameid = update_assetid(assetids[i]);
        if ( nameid >= 0 && assetname != 0 && strcmp(assetname,ASSETNAMES.list[nameid]) == 0 )
            assetidstr = clonestr(assetids[i]);
        myfree(assetids[i],"62");
    }
    Numinlist = 0;
    myfree(assetids,"63");
    return(assetidstr);
}

#endif
