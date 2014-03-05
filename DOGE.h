
//  Created by jl777
//  MIT License
//

#ifndef gateway_gateway_h
#define gateway_gateway_h


//#define MANUAL_THRESHOLD  100000  // any transactions over this size require manual confirmation, by all gateways
#define COINID 1
#define COINNAME "DOGE"
#define COINASSET "7761388364129412234"
#define milliCOINASSET "6572437125760810791"
#define milliCOINNAME "milliDOGE"

#define DAEMON "dogecoind"  // for most bitcoin forks, changing TXFEE and gateway specific defines should be enough
#define TXFEE 1.0000           // don't forget to match txfee with coin
#define MAX_VOUTS 8       
#define MIN_CONFIRMS 3
#define DEPOSIT_FREQUENCY 3
#define POLL_SECONDS 60
#define WALLETBACKUP "wallet.DOGE"

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


#endif
