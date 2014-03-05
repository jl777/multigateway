
//  Created by jl777
//  MIT License
//


#ifndef gateway_NXTassets_h
#define gateway_NXTassets_h

struct strings ASSETNAMES,ASSETCHANGES; 

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
