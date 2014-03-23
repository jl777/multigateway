
//  Created by jl777
//  MIT License
//

#ifndef gateway_NXTutils_h
#define gateway_NXTutils_h

#include <string.h>

#define NO_DEBUG
long MY_ALLOCATED,NUM_ALLOCATED,MAX_ALLOCATED,*MY_ALLOCSIZES; void **PTRS;

void *mymalloc(long allocsize)
{
    void *ptr;
    MY_ALLOCATED += allocsize;
    ptr = malloc(allocsize);
    memset(ptr,0,allocsize);
#ifdef NO_DEBUG
    return(ptr);
#endif
    if ( NUM_ALLOCATED >= MAX_ALLOCATED )
    {
        MAX_ALLOCATED += 100;
        PTRS = realloc(PTRS,MAX_ALLOCATED * sizeof(*PTRS));
        MY_ALLOCSIZES = realloc(MY_ALLOCSIZES,MAX_ALLOCATED * sizeof(*MY_ALLOCSIZES));
    }
    MY_ALLOCSIZES[NUM_ALLOCATED] = allocsize;
    printf("%ld myalloc.%p %ld | %ld\n",NUM_ALLOCATED,ptr,allocsize,MY_ALLOCATED);
    PTRS[NUM_ALLOCATED++] = ptr;
    return(ptr);
}

void myfree(void *ptr,char *str)
{
    int i;
#ifdef NO_DEBUG
    free(ptr);
    return;
#endif
    //printf("%s: myfree.%p\n",str,ptr);
    for (i=0; i<NUM_ALLOCATED; i++)
    {
        if ( PTRS[i] == ptr )
        {
            MY_ALLOCATED -= MY_ALLOCSIZES[i];
            printf("%s: freeing %d of %ld | %ld\n",str,i,NUM_ALLOCATED,MY_ALLOCATED);
            if ( i != NUM_ALLOCATED-1 )
            {
                MY_ALLOCSIZES[i] = MY_ALLOCSIZES[NUM_ALLOCATED-1];
                PTRS[i] = PTRS[NUM_ALLOCATED-1];
            }
            NUM_ALLOCATED--;
            free(ptr);
            return;
        }
    }
    printf("couldn't find %p in PTRS[%ld]??\n",ptr,NUM_ALLOCATED);
    while ( 1 ) sleep(1);
    free(ptr);
}

int copy_str(char *dest,char *src,int max)
{
    int j,i = 0;
    for (j=0; i<max; i++,j++)
        if ( (dest[j]= src[i]) == ',' || src[i] == '"' )
            break;
    dest[j] = 0;
    //printf("copy_str(%s).%d\n",dest,j);
    return(j);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        printf("warning cloning nullstr.%p\n",str); while ( 1 ) sleep(1);
        str = "<nullstr>";
    }
    clone = mymalloc(strlen(str)+1);
    strcpy(clone,str);
    return(clone);
}

int unhex(char c)
{
    if ( c >= '0' && c <= '9' )
        return(c - '0');
    else if ( c >= 'a' && c <= 'f' )
        return(c - 'a' + 10);
    else return(0);
}

unsigned char _decode_hex(char *hex)
{
    return((unhex(hex[0])<<4) | unhex(hex[1]));
}

void decode_hex(unsigned char *bytes,int n,char *hex)
{
    int i;
    for (i=0; i<n; i++)
        bytes[i] = _decode_hex(&hex[i*2]);
}

char hexbyte(int c)
{
    if ( c < 10 )
        return('0'+c);
    else return('a'+c-10);
}

int init_hexbytes(char *hexbytes,unsigned char *message,long len)
{
    int i,lastnonz = -1;
    for (i=0; i<len; i++)
    {
        if ( message[i] != 0 )
        {
            lastnonz = i;
            hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
            hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
        }
        else hexbytes[i*2] = hexbytes[i*2+1] = '0';
        //printf("i.%d (%02x) [%c%c] last.%d\n",i,message[i],hexbytes[i*2],hexbytes[i*2+1],lastnonz);
    }
    lastnonz++;
    hexbytes[lastnonz*2] = 0;
    return(lastnonz*2+1);
}

char *load_file(char *fname,char **bufp,int64_t  *lenp,int64_t  *allocsizep)
{
    FILE *fp;
    int64_t  filesize,buflen = *allocsizep;
    char *buf = *bufp;
    *lenp = 0;
    //printf("LOAD(%s) buf.%p alloc.%lld\n",fname,buf,buflen);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        filesize = ftell(fp);
        if ( filesize == 0 )
        {
            fclose(fp);
            *lenp = 0;
            return(0);
        }
        if ( filesize > buflen-1 )
        {
            *allocsizep = filesize+1;
            *bufp = buf = realloc(buf,*allocsizep);
            buflen = filesize+1;
        }
        rewind(fp);
        if ( buf == 0 )
            printf("Null buf ???\n");
        else
        {
            fread(buf,1,filesize,fp);
            buf[filesize] = 0;
        }
        fclose(fp);
        
        *lenp = filesize;
        //printf("filesize.%lld\n",filesize);
    }
    return(buf);
}

void purge_ptrs(char **ptrs,int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        if ( ptrs[i] != 0 )
            myfree(ptrs[i],"99"), ptrs[i] = 0;
    }
}

char *strip_tohexcodes(char *ptr)
{
    int n;
    if ( ptr == 0 )
        return(0);
    if ( *ptr == '"' )
        ptr++;
    n = (int)strlen(ptr);
    while ( n>2 && (ptr[n-1] == ',' || ptr[n-1] == '"' || ptr[n-1] == ' ' || ptr[n-1] == '\r' || ptr[n-1] == '\n') )
        n--;
    ptr[n] = 0;
    return(ptr);
}

/*void purge_strings(struct strings *ptrs)
{
    int i;
    for (i=0; i<ptrs->num; i++)
    {
        if ( ptrs->list[i] != 0 )
            free(ptrs->list[i]);
        if ( ptrs->argptrs[i] != 0 )
            free(ptrs->argptrs[i]);
        ptrs->list[i] = 0;
        ptrs->argptrs[i] = 0;
        ptrs->args[i] = 0;
        ptrs->arg2[i] = 0;
    }
    ptrs->num = 0;
}

int remove_string(struct strings *ptrs,char *str)
{
    int i;

    printf("remove_string.(%s)\n",str);
    for (i=0; i<ptrs->num; i++)
    {
        // printf("i.%d of %d: %s vs %s\n",i,ptrs->num,ptrs->list[i],str);
        if ( strcmp(ptrs->list[i],str) == 0 )
        {
            free(ptrs->list[i]);
            if ( ptrs->argptrs[i] != 0 )
                free(ptrs->argptrs[i]);
            if ( i != ptrs->num-1 )
            {
                ptrs->list[i] = ptrs->list[ptrs->num-1];
                ptrs->argptrs[i] = ptrs->argptrs[ptrs->num-1];
                ptrs->args[i] = ptrs->args[ptrs->num-1];
                ptrs->arg2[i] = ptrs->arg2[ptrs->num-1];
            }
            ptrs->num--;
            //printf("removed (%s) -> %d left\n",str,ptrs->num);
            return(ptrs->num);
       }
    }
    printf("remove_string unexpected missing %s\n",str);// while ( 1 ) sleep(1);
    return(ptrs->num);
}*/

int find_string(struct strings *ptrs,char *str)
{
    int i;
    for (i=0; i<ptrs->num; i++)
    {
        //printf("i.%d of %d: %s vs %s\n",i,ptrs->num,ptrs->list[i],str);
        if ( strcmp(ptrs->list[i],str) == 0 )
            return(i);
    }
    //printf("couldnt find (%s) from %d\n",str,ptrs->num);
    return(-1);
}

int add_string(struct strings *ptrs,char *str,int64_t arg,int64_t arg2,void *argptr,void *argptr2)
{
    int i;
    if ( find_string(ptrs,str) >= 0 )
        return(0);
    //printf("add string.%d (%s)\n",ptrs->num,str);
    i = ptrs->num;
    if ( i >= ptrs->max )
    {
        ptrs->max += 10;
        ptrs->list = realloc(ptrs->list,sizeof(*ptrs->list) * ptrs->max);
        ptrs->argptrs = realloc(ptrs->argptrs,sizeof(*ptrs->argptrs) * ptrs->max);
        ptrs->argptrs2 = realloc(ptrs->argptrs2,sizeof(*ptrs->argptrs2) * ptrs->max);
        ptrs->args = realloc(ptrs->args,sizeof(*ptrs->args) * ptrs->max);
        ptrs->arg2 = realloc(ptrs->arg2,sizeof(*ptrs->arg2) * ptrs->max);
    }
    ptrs->list[i] = clonestr(str);
    ptrs->args[i] = arg;
    ptrs->arg2[i] = arg2;
    ptrs->argptrs[i] = argptr;
    ptrs->argptrs2[i] = argptr2;
    ptrs->num++;
    return(ptrs->num);
}

#endif
