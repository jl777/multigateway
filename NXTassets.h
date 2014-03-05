//  Created by jl777
//  MIT License
//


#ifndef gateway_NXTassets_h
#define gateway_NXTassets_h

struct strings ASSETNAMES,ASSETCHANGES; 

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

#endif
