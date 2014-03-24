//
//  Created by jl777
//  MIT License
//


#ifndef gateway_NXTcrypto_h
#define gateway_NXTcrypto_h

#include <stdio.h>
#include "crypto_box.h"
#include "randombytes.h"

//#ifdef IS_GATEWAY
#define MAX_SESSIONS 1000
int Session_ID;
unsigned char Public_keys[MAX_SESSIONS][crypto_box_SECRETKEYBYTES];
unsigned char Private_keys[MAX_SESSIONS][crypto_box_SECRETKEYBYTES];
//#else
unsigned char ESCROW_PUBLICKEYS[MAX_SESSIONS][crypto_box_PUBLICKEYBYTES];
unsigned char PUBLICKEYS[MAX_SESSIONS][crypto_box_PUBLICKEYBYTES];
unsigned char PRIVATEKEYS[MAX_SESSIONS][crypto_box_PUBLICKEYBYTES];
//#endif

#ifdef TEST_CRYPTO
#define crypto_box_curve25519xsalsa20poly1305_ref_PUBLICKEYBYTES 32
#define crypto_box_curve25519xsalsa20poly1305_ref_SECRETKEYBYTES 32
#define crypto_box_curve25519xsalsa20poly1305_ref_BEFORENMBYTES 32
#define crypto_box_curve25519xsalsa20poly1305_ref_NONCEBYTES 24
#define crypto_box_curve25519xsalsa20poly1305_ref_ZEROBYTES 32
#define crypto_box_curve25519xsalsa20poly1305_ref_BOXZEROBYTES 16

unsigned char alicesk[crypto_box_SECRETKEYBYTES];
unsigned char alicepk[crypto_box_PUBLICKEYBYTES];
unsigned char bobsk[crypto_box_SECRETKEYBYTES];
unsigned char bobpk[crypto_box_PUBLICKEYBYTES];
unsigned char n[crypto_box_NONCEBYTES];
unsigned char m[10000];
unsigned char c[10000];
unsigned char m2[10000];

void crypto_tests()
{
    int mlen;
    int i;
    
    for (mlen = 0;mlen < 1000 && mlen + crypto_box_ZEROBYTES < sizeof m;++mlen)
    {
        crypto_box_keypair(alicepk,alicesk);
        crypto_box_keypair(bobpk,bobsk);
        randombytes(n,crypto_box_NONCEBYTES);
        randombytes(m + crypto_box_ZEROBYTES,mlen);
        crypto_box(c,m,mlen + crypto_box_ZEROBYTES,n,bobpk,alicesk);
        if ( crypto_box_open(m2,c,mlen + crypto_box_ZEROBYTES,n,alicepk,bobsk) == 0)
        {
            for (i = 0;i < mlen + crypto_box_ZEROBYTES;++i)
                if ( m2[i] != m[i] )
                {
                    printf("bad decryption\n");
                    break;
                }
            printf("%d ",i);
        }
        else printf("ciphertext fails verification\n");
    }  
    printf("done with crypto_tests\n"); getchar();
}
#endif

char *AM_new_session(int timestamp,int sessionid)
{
    struct gateway_AM AM;
    set_standard_AM(&AM,START_NEW_SESSION,NXTACCTA,timestamp);
    AM.sessionid = sessionid;
    sessionid %= MAX_SESSIONS;
    crypto_box_keypair(Public_keys[sessionid],Private_keys[sessionid]);
    memcpy(AM.publickey,Public_keys[sessionid],sizeof(AM.publickey));
    int i;
    for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
        printf("%02x ",Public_keys[sessionid][i]);
    printf("actual escrow pubkey\n");
    return(submit_AM(&AM));
}

char *AM_anonymous_payments(int timestamp,int gatewayid,char *nxtaddr,unsigned char *userpubkey,unsigned char *paymentkey_by_prevrecv,unsigned char *payload_by_escrow,unsigned char *nonce)
{
    struct gateway_AM AM;
    set_standard_AM(&AM,SEND_ANONYMOUS_PAYMENTS,nxtaddr,timestamp);
    AM.gatewayid = gatewayid;
    memcpy(AM.publickey,userpubkey,sizeof(AM.publickey));
    memcpy(AM.nonce,nonce,sizeof(AM.nonce));
    memcpy(AM.paymentkey_by_prevrecv,paymentkey_by_prevrecv,sizeof(AM.paymentkey_by_prevrecv));
    memcpy(AM.payload_by_escrow,payload_by_escrow,sizeof(AM.payload_by_escrow));
    return(submit_AM(&AM));
}

void set_payment_bundle(struct payment_bundle *payment,int sessionid,unsigned char *escrow_pubkey)
{
    if ( sessionid == 0 )
    {
        memset(payment,0,sizeof(*payment));
        payment->sessionid = sessionid;
        memcpy(payment->escrow_pubkey,escrow_pubkey,sizeof(payment->escrow_pubkey));
        strcpy((char *)payment->depositaddr,"");
        strcpy((char *)payment->paymentacct_key,"");
        strcpy((char *)payment->txouts[0],"");
        payment->amounts[0] = 7;
    }
}

void broadcast_anonymous(int timestamp,int sessionid,char *NXTaddr)
{
    int i;
    char *retstr;
    struct payment_bundle payment;
    unsigned char nonce[crypto_box_NONCEBYTES];
    unsigned char paymentkey_by_prevrecv[crypto_box_PUBLICKEYBYTES + crypto_box_SECRETKEYBYTES + crypto_box_ZEROBYTES];
    unsigned char cipher[crypto_box_SECRETKEYBYTES + crypto_box_ZEROBYTES];
    unsigned char payload_by_escrow[sizeof(struct payment_bundle) + crypto_box_ZEROBYTES];
    unsigned char cipher2[sizeof(struct payment_bundle) + crypto_box_ZEROBYTES];
    
    memset(paymentkey_by_prevrecv,0,sizeof(paymentkey_by_prevrecv));
    memset(payload_by_escrow,0,sizeof(payload_by_escrow));
    memset(cipher,0,sizeof(cipher));
    memset(cipher2,0,sizeof(cipher2));
    sessionid %= MAX_SESSIONS;
    crypto_box_keypair(PUBLICKEYS[sessionid],PRIVATEKEYS[sessionid]);
    randombytes(nonce,crypto_box_NONCEBYTES);
    
    randombytes(paymentkey_by_prevrecv + crypto_box_ZEROBYTES,crypto_box_SECRETKEYBYTES);
    randombytes(payload_by_escrow + crypto_box_ZEROBYTES,sizeof(struct payment_bundle));
    randombytes((unsigned char *)&payment,sizeof(payment));
    
    if ( sessionid > 0 )
    {
        // search user's pubkeys for sessionid-1 and if recv exists and payment acct exists -> paymentkey_by_recv
    }
    set_payment_bundle(&payment,sessionid,ESCROW_PUBLICKEYS[sessionid]);
    
    memcpy(payload_by_escrow + crypto_box_ZEROBYTES,&payment,sizeof(payment));
    crypto_box(cipher,paymentkey_by_prevrecv,crypto_box_PUBLICKEYBYTES + crypto_box_SECRETKEYBYTES + crypto_box_ZEROBYTES,nonce,ESCROW_PUBLICKEYS[sessionid],PRIVATEKEYS[sessionid]);
    crypto_box(cipher2,payload_by_escrow,sizeof(struct payment_bundle) + crypto_box_ZEROBYTES,nonce,ESCROW_PUBLICKEYS[sessionid],PRIVATEKEYS[sessionid]);
    
    retstr = AM_anonymous_payments(timestamp,0,NXTaddr,PUBLICKEYS[sessionid],cipher,cipher2,nonce);
    for (i=0; i<crypto_box_NONCEBYTES; i++)
        printf("%02x ",nonce[i]);
    printf("got nonce\n");
    for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
        printf("%02x ",PUBLICKEYS[sessionid][i]);
    printf("%s pubkey\n",NXTaddr);
    for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
        printf("%02x ",ESCROW_PUBLICKEYS[sessionid][i]);
    printf("actual escrow pubkey\n");
    getchar();
    if ( retstr != 0 )
    {
        printf("%s AM_anonymous_payments returns (%s)\n",NXTaddr,retstr);
        free(retstr);
    }
}

void process_anonymous_payments(int timestamp,struct gateway_AM *ap)
{
    int64_t sum = 0;
    int i,n,sessionid = ap->sessionid % MAX_SESSIONS;
    struct payment_bundle payment;
    unsigned char payment_bytes[sizeof(payment) + crypto_box_ZEROBYTES];
    printf("call crypto_box_open\n");
    for (i=0; i<crypto_box_NONCEBYTES; i++)
        printf("%02x ",ap->nonce[i]);
    printf("got nonce\n");
    for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
        printf("%02x ",ap->publickey[i]);
    printf("%s pubkey\n",ap->NXTaddr);
    for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
        printf("%02x ",Public_keys[sessionid][i]);
    printf("actual escrow pubkey\n");
    //getchar();
    if ( crypto_box_open(payment_bytes,ap->payload_by_escrow,sizeof(payment) + crypto_box_ZEROBYTES,ap->nonce,ap->publickey,Private_keys[sessionid]) == 0)
    {
        memcpy(&payment,payment_bytes + crypto_box_ZEROBYTES,sizeof(payment));
        for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
            printf("%02x ",payment.escrow_pubkey[i]);
        printf("got escrow pubkey\n");
        //getchar();
        if ( memcmp(payment.escrow_pubkey,Public_keys[sessionid],sizeof(payment.escrow_pubkey)) == 0 )
        {
            for (i=n=0; i<(int)(sizeof(payment.amounts)/sizeof(*payment.amounts)); i++)
            {
                if ( payment.amounts[i] != 0 )
                {
                    printf("(%.0f -> %s) ",(double)payment.amounts[i],payment.txouts[i]);
                    n++;
                    sum += payment.amounts[i];
                }
            }
            printf("%d payments from %s totalling %.0f\n",n,ap->NXTaddr,(double)sum);
        }
        else printf("public key from %s didnt match sessionid.%d\n",ap->NXTaddr,sessionid);
    }
    else printf("error crypto_box_open from %s\n",ap->NXTaddr);
}

#endif
