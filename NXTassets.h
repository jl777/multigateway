
//  Created by jl777
//  MIT License
//


#ifndef gateway_NXTassets_h
#define gateway_NXTassets_h

struct strings ASSETNAMES,ASSETCHANGES; 
struct active_NXTacct **NXTaccts; int Numactive,Maxactive;

int validate_nxtaddr(char *nxtaddr)
{
    // make sure it has publickey
    int n = (int)strlen(nxtaddr);
    while ( n > 10 && (nxtaddr[n-1] == '\n' || nxtaddr[n-1] == '\r') )
        n--;
    if ( n < 8 )
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
    if ( strlen(nxtaddr) < 5 )
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

void update_asset_balances(char *change_txid,char *asset,char *recipientstr,char *senderstr,int64_t satoshis)
{
    int assetid;
    struct active_NXTacct *receiver,*sender;
    receiver = get_active_NXTacct(recipientstr);
    sender = get_active_NXTacct(senderstr);
    if ( receiver == 0 || sender == 0 )
    {
        printf("couldnt find %s or %s\n",recipientstr,senderstr);
        return;
    }
    add_string(&ASSETNAMES,asset,0,0,0,0);
    verify_assetspace(receiver,ASSETNAMES.num);
    verify_assetspace(sender,ASSETNAMES.num);
    assetid = find_string(&ASSETNAMES,asset);
    //printf("assetid.%d %s -> %s, %.8f\n",assetid,senderstr,recipientstr,(double)satoshis/SATOSHIDEN);
    if ( assetid >= 0 )
    {
        if ( add_string(&ASSETCHANGES,change_txid,assetid,satoshis,receiver,sender) != 0 )
        {
            if ( strcmp(GENESISACCT,senderstr) != 0 && sender->balances[assetid] < satoshis )
                printf("ILLEGAL transfer %s assetid.%d to %s | %s.(%.8f) < xfer.%8f????\n",ASSETNAMES.list[assetid],assetid,recipientstr,senderstr,(double)sender->balances[assetid]/SATOSHIDEN,(double)satoshis/SATOSHIDEN);
            else
            {
                receiver->balances[assetid] += satoshis;
                sender->balances[assetid] -= satoshis;
                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> transfer %s assetid.%d %.8f from %s.(%.8f) to %s.(%.8f)\n",ASSETNAMES.list[assetid],assetid,(double)satoshis/SATOSHIDEN,senderstr,(double)sender->balances[assetid]/SATOSHIDEN,recipientstr,(double)receiver->balances[assetid]/SATOSHIDEN);
            }
        }
    }
}

int process_NXTtransaction(char *nxt_txid)
{
    static int timestamp = 0;
    struct gateway_AM AM;
    struct active_NXTacct *active;
    char cmd[4096],*jsonstr,*retstr,*cmpstr;
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
        if ( retstr != 0 )
        {
            n = (int)strlen(Message) / 2;
            memset(&AM,0,sizeof(AM));
            decode_hex((void *)&AM,n,Message);
            //for (j=0; j<n; j++)
            //    printf("%02x",((char *)&AM)[j]&0xff);
            //printf("timestamp.%s %d\n",Timestamp,timestamp);
            if ( is_gateway_AM(&gatewayid,&AM,Sender,Recipient) != 0 )
            {
                // if ( atoi(Confirmations) < MIN_NXTCONFIRMS ) need to make a queue to check NXT confirms
                //    queue_AM_txid(nxt_txid);
                //else
                {
                    flag++;
                    update_gateway_states(timestamp,&AM);
                }
            }
            free(retstr);
        }
        if ( flag == 0 )
        {
            mult = 0;
            quantity = atoi(Quantity);
            if ( strcmp(Asset,COINASSET) == 0 )
                mult = SATOSHIDEN;
            else if ( strcmp(Asset,milliCOINASSET) == 0 )
                mult = (SATOSHIDEN / 1000);
            if ( quantity > 0 )
            {
                //printf("%d descr.(%s) %s -> %s, QTY %.8f\n",strcmp(Recipient,GENESISACCT),Description,Sender,Recipient,(double)(quantity)/SATOSHIDEN);
                if ( strcmp(Recipient,GENESISACCT) == 0 )
                {
                    if ( Description[0] != 0 )
                    {
                        if ( strcmp(COINNAME,Name) == 0 )
                            strcpy(Asset,COINASSET);
                        else if ( strcmp(milliCOINNAME,Name) == 0 )
                            strcpy(Asset,milliCOINASSET);
                        else Asset[0] = 0;
                        if ( Asset[0] != 0 )
                            update_asset_balances(nxt_txid,Asset,Sender,Recipient,quantity * SATOSHIDEN);
                    }
                }
                else if ( mult > 0 )
                    update_asset_balances(nxt_txid,Asset,Recipient,Sender,quantity * SATOSHIDEN);
                //printf("ASSET.(%s)\n",tmpstr);
#ifdef IS_GATEWAY
                for (gatewayid=0; gatewayid<=NUM_GATEWAYS; gatewayid++)
                {
                    cmpstr = (gatewayid < NUM_GATEWAYS) ? Gateway_NXTaddrs[gatewayid] : NXTISSUERACCT;
                    if ( strcmp(cmpstr,Recipient) == 0 )
                    {
                        printf(">>>>> ASSET XFER getTransaction.%s %s\n",nxt_txid,tmpstr);
                        if ( (active= get_active_NXTacct(Sender)) != 0 )
                            queue_asset_redemption(timestamp,gatewayid % NUM_GATEWAYS,active,Recipient,nxt_txid,quantity * mult,Asset);
                        else printf("NXTaddr.%s without account sent quantity.%s\n",Sender,Quantity);
                    }
                }
#endif
            }
        }
        free(jsonstr);
    }
    return(timestamp);
}

int get_blocks_to_process(int timestamp)
{
    int i,tmp,AMcount = 0;
    Numinlist = 0;
    issue_getAccountTransactionIds(addto_NXTlist,NXTISSUERACCT,timestamp);
    for (i=0; i<Numinlist; i++)
    {
        //printf("(%d %s) ",i,List[i]);
        if ( (tmp= process_NXTtransaction(List[i])) != 0 )
        {
            if ( tmp > timestamp )
                timestamp = tmp;
            AMcount++;
        }
        free(List[i]);
        List[i] = 0;
    }
    Numinlist = 0;
    return(timestamp);
}

#endif
