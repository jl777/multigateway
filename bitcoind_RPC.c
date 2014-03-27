//
//  bitcoind_RPC.c
//  Created by jl777, Mar 27, 2014
//  MIT License
//
// based on example from http://curl.haxx.se/libcurl/c/getinmemory.html and util.c from cpuminer.c

#ifndef JL777_BITCOIND_RPC
#define JL777_BITCOIND_RPC

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <curl/curl.h>
#include <curl/easy.h>

struct upload_buffer { const void *buf; size_t len; };
struct MemoryStruct { char *memory; size_t size; };

static size_t WriteMemoryCallback(void *ptr,size_t size,size_t nmemb,void *data)
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

static size_t upload_data_cb(void *ptr,size_t size,size_t nmemb,void *user_data)
{
    struct upload_buffer *ub = user_data;
    int len = (int)(size * nmemb);
    if ( len > ub->len )
        len = (int)ub->len;
    if ( len != 0 )
    {
        memcpy(ptr,ub->buf,len);
        ub->buf += len;
        ub->len -= len;
    }
    return len;
}

char *bitcoind_RPC(int numretries,char *url,char *userpass,char *command,char *args)
{
    char *retstr,*quote;
    CURL *curl_handle;
    CURLcode res;
    char len_hdr[1024],databuf[1024];
    struct curl_slist *headers = NULL;
    struct upload_buffer upload_data;
    struct MemoryStruct chunk;
    if ( args == 0 )
        args = "";
retry:
    chunk.memory = malloc(1);     // will be grown as needed by the realloc above
    chunk.size = 0;                 // no data at this point
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYHOST,0);
    curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYPEER,0);
    curl_easy_setopt(curl_handle,CURLOPT_URL,url);
    curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,WriteMemoryCallback); // send all data to this function
    curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void *)&chunk); // we pass our 'chunk' struct to the callback function
    curl_easy_setopt(curl_handle,CURLOPT_USERAGENT,"libcurl-agent/1.0"); // some servers don't like requests that are made without a user-agent field, so we provide one
    if ( userpass != 0 )
        curl_easy_setopt(curl_handle,CURLOPT_USERPWD,userpass);
    if ( command != 0 )
    {
        curl_easy_setopt(curl_handle,CURLOPT_READFUNCTION,upload_data_cb);
        curl_easy_setopt(curl_handle,CURLOPT_READDATA,&upload_data);
        curl_easy_setopt(curl_handle,CURLOPT_POST,1);
        if ( args[0] != 0 )
            quote = "\"";
        else quote = "";
        sprintf(databuf,"{\"id\":\"jl777\",\"method\":\"%s\",\"params\":[%s%s%s]}",command,quote,args,quote);
        upload_data.buf = databuf;
        upload_data.len = strlen(databuf);
        sprintf(len_hdr, "Content-Length: %lu",(unsigned long)upload_data.len);
        headers = curl_slist_append(headers,"Content-type: application/json");
        headers = curl_slist_append(headers,len_hdr);
        curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,headers);
    }
    res = curl_easy_perform(curl_handle);
    if ( res != CURLE_OK )
    {
        fprintf(stderr, "curl_easy_perform() failed: %s (%s %s %s %s)\n",curl_easy_strerror(res),url,userpass,command,args);
        sleep(30);
        if ( numretries-- > 0 )
        {
            free(chunk.memory);
            curl_easy_cleanup(curl_handle);
            goto retry;
        }
    }
    else
    {
        // printf("%lu bytes retrieved [%s]\n", (int64_t )chunk.size,chunk.memory);
    }
    curl_easy_cleanup(curl_handle);
    retstr = malloc(strlen(chunk.memory)+1);
    strcpy(retstr,chunk.memory);
    free(chunk.memory);
    return(retstr);
}

#endif

