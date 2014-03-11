//  Created by jl777
//  MIT License
//


#ifndef gateway_nodecoin_h
#define gateway_nodecoin_h

#define NODECOIN "1515935197377598405"
#define NODECOIN_BLOCKRATE 100
#define NODECOIN_MINPAYOUT 10

struct strings IPADDRS;
struct strings PEERS;    // for nodecoin

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

int update_ipaddr(char *ipaddr)
{
    int ind = -1;;
    ind = find_string(&IPADDRS,ipaddr);
    if ( ind < 0 )
    {
        add_string(&IPADDRS,ipaddr,0,0,0,0);
        printf("new IPADDRS.(%s)\n",ipaddr);
    }
    ind = find_string(&IPADDRS,ipaddr);
    if ( ind < 0 )
    {
        printf("FATAL ERROR: failed adding IPADDRS.(%s)\n",ipaddr);
        while ( 1 ) sleep(1);
    }
    return(ind);
}

void process_nodecoin_packet(struct server_request *req,char *clientip)
{
    struct server_response *rp = (struct server_response *)req;
    struct active_NXTacct *active,*client;
    int64_t sum,val;
    int i,j,ind,nonz = 0;
    if ( strncmp(clientip,"::ffff:",strlen("::ffff:")) == 0 )
        clientip += strlen("::ffff:");
    client = get_active_NXTacct(req->NXTaddr);
    ind = update_ipaddr(clientip);
    if ( ind >= 0 && client != 0 )
    {
        if ( strcmp(client->ipaddr,clientip) != 0 )
        {
            printf("NXT.%s updating IP from (%s) -> (%s)\n",req->NXTaddr,client->ipaddr,clientip);
            strcpy(client->ipaddr,clientip);
            IPADDRS.argptrs[ind] = client;
        }
    }
    sum = 0;
    for (i=0; i<MAX_ACTIVE_PEERS; i++)
    {
        if ( req->peers[i].ipaddr[0] != 0 )
        {
            ind = update_ipaddr(req->peers[i].ipaddr);
            if ( (active= IPADDRS.argptrs[ind]) == 0 )
            {
                for (j=0; j<Numactive; j++)
                {
                    active = NXTaccts[j];
                    if ( active->ipaddr[0] == 0 )//strcmp(active->ipaddr,req->peers[i].ipaddr) != 0 )
                    {
                        printf("BIND.%d (%s to NXT.%s)\n",ind,req->peers[i].ipaddr,active->NXTaddr);
                        IPADDRS.argptrs[ind] = active;
                        strcpy(active->ipaddr,req->peers[i].ipaddr);
                        break;
                    }
                }
            }
            if ( req->peers[i].downloaded+req->peers[i].downloaded != 0 )
            {
                sum++;
                IPADDRS.args[ind] += 1;
                val = (req->peers[i].uploaded + req->peers[i].downloaded) / 10000;
                if ( val > 1 )
                    val = 1;
                IPADDRS.arg2[ind] += val;
            }
            if ( (active= IPADDRS.argptrs[ind]) != 0 && strcmp(active->ipaddr,req->peers[i].ipaddr) == 0 )
            {
                active->nodeshares = (IPADDRS.args[ind] + 0*IPADDRS.arg2[ind]);
                IPADDRS.args[ind] = 0;
                IPADDRS.arg2[ind] = 0;
                //printf("ind.%d NXT.%s %s %s %.8f shares, %.8f nodecoins\n",ind,active->NXTaddr,active->ipaddr,req->peers[i].ipaddr,(double)active->nodeshares/SATOSHIDEN,(double)active->nodecoins/SATOSHIDEN);
            }
            nonz++;
            printf("(%s %.0f %.0f) ",req->peers[i].ipaddr,(double)req->peers[i].uploaded,(double)req->peers[i].downloaded);
        }
    }
    if ( client != 0 && sum != 0 )
        client->nodeshares++;
    printf("[(%s) NXT.%s reports nonz.%d]\n",clientip,req->NXTaddr,nonz);
    ind = update_ipaddr(clientip);
    printf("%s -> ind.%d %p\n",clientip,ind,IPADDRS.argptrs[ind]);
    if ( ind >= 0 && (active= IPADDRS.argptrs[ind]) != 0 )//&& strcmp(active->ipaddr,clientip) == 0 )
    {
        rp->nodeshares = active->nodeshares;
        rp->current_nodecoins = active->current_nodecoins;
        rp->nodecoins = active->nodecoins;
        rp->nodecoins_sent = active->nodecoins_sent;
        printf("%s: %s vs %s %ld %ld %ld\n",active->NXTaddr,active->ipaddr,clientip,(long)rp->nodeshares,(long)rp->current_nodecoins,(long)rp->nodecoins);
    }
}

void payout_nodecoins()
{
    int j;
    double dsum;
    char *transfer_txid;
    int64_t sum,sum2,frac,satoshis;
    struct active_NXTacct *active;
    dsum = 0.;
    sum = sum2 = 0;
    for (j=0; j<Numactive; j++)
        sum += NXTaccts[j]->nodeshares;
    if ( sum != 0 )
    {
        for (j=0; j<Numactive; j++)
        {
            active = NXTaccts[j];
            active->nodecoins += active->current_nodecoins;
            active->current_nodecoins = (int64_t)(((double)SATOSHIDEN * NODECOIN_BLOCKRATE * active->nodeshares) / sum);
            sum2 += active->current_nodecoins;
            dsum += (double)(NODECOIN_BLOCKRATE * active->nodeshares) / sum;
            active->nodeshares = 0;
            if ( active->current_nodecoins != 0 )
                printf("%.8f ",(double)active->current_nodecoins/SATOSHIDEN);
            if ( Deadman_switch < (60/POLL_SECONDS)+1 && active->nodecoins >= NODECOIN_MINPAYOUT*SATOSHIDEN )
            {
                frac = (active->nodecoins % SATOSHIDEN);
                satoshis = active->nodecoins - frac;
                transfer_txid = issue_transferAsset(NXTACCTSECRET,active->NXTaddr,NODECOIN,satoshis/SATOSHIDEN,MIN_NXTFEE,1440,0);
                if ( transfer_txid != 0 )
                {
                    printf("txid.%s NXT.%s <- %.8f nodecoins\n",transfer_txid,active->NXTaddr,(double)satoshis/SATOSHIDEN);
                    myfree(transfer_txid,"333");
                    active->nodecoins_sent += satoshis;
                    active->nodecoins -= satoshis;
                }
            }
        }
        printf("sum %ld sum2 %ld dsum %.8f\n",(long)sum,(long)sum2,dsum);
    }
}

void nodecoin_loop(char *NXTaddr,int loopflag)
{
    char *retstr;
    int i,peerid,numactivepeers;
    struct server_request R;
    struct server_response *rp = (struct server_response *)&R;
    struct peer_info peers[MAX_ACTIVE_PEERS];
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
            memcpy(R.peers,peers,sizeof(peers));
            if ( issue_server_request(&R,0) == sizeof(struct server_response) )
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
            else printf("error submitting results\n");
        }
        if ( loopflag == 0 )
            break;
        sleep(10);
    }
}

#endif
