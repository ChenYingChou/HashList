// hash.cc
// vim: set ts=4 sw=4 et:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#if defined(CHARPTR_VER)
  #define SUPPORT_HASHLIST_CHARPTR_STRDUP   1
#endif

#include "HashList.h"

using namespace std;
using namespace tony;

#include <stdarg.h>

//#define STRING_VER 1
//#define CHARPTR_VER 1
//#define INTEGER_VER 1

#if defined(STRING_VER)
  typedef THashList<string> HashList;
#elif defined(CHARPTR_VER)
  typedef THashList<char*>  HashList;
#elif defined(INTEGER_VER)
  typedef THashList<int>        HashList;
#else
  #error Need STRING_VER, CHARPTR_VER or INTEGER_VER to be defined!
#endif

static void MemUsage ( )
{
    char buf[256];
    snprintf(buf,sizeof(buf),"/proc/%u/statm",(unsigned)getpid());
    FILE* fp = fopen(buf,"r");
    if (fp) {
        // unit is per-block 4K
        unsigned size;      // total program size
        unsigned resident;  // resident set size
        unsigned share;     // shared pages
        unsigned text;      // text (code)
        unsigned lib;       // library
        unsigned data;      // data/stack
        unsigned dt;        // dirty pages (unused in Linux 2.6)
        double xVM, xRSS;
        fscanf(fp, "%u %u"/* %u %u %u %u"*/,&size,&resident/*, &share, &text, &lib, &data*/);
        xVM = (double)size / 256;
        xRSS = (double)resident / 256;
        printf("VM=%.2fMB, RSS=%.2fMB memory used\n",xVM,xRSS);
        fclose(fp);
    }
}

static void Load(HashList& X, const char *FileName)
{
    FILE *fp = fopen(FileName,"r");
    if (fp == NULL) {
        printf("Can't read file: %s.\n",FileName);
        return;
    }

    char s[1024];
    s[sizeof(s)-1] = 0;
    
    int cntAdd = 0;
    int cntDup = 0;

    while (fgets(s,sizeof(s),fp) != NULL) {
        char *p = &s[strlen(s)];
        while (p > s && (unsigned char)p[-1] <= ' ') p--;
        p[0] = 0;
        p[1] = 0;
        if (p > s) {
            p = s;
            while (*p != 0 && (*p != ' ' && *p != ':')) p++;
            *p++ = 0;
            while (*p != 0 && (unsigned char)p[0] <= ' ') p++;

            char *t = s;
            while (*t != 0 && (unsigned char)t[0] <= ' ') t++;
            
        #if defined(STRING_VER) || defined(CHARPTR_VER)
            if (X.Add(t,p)) {
        #elif defined(INTEGER_VER)
            if (X.Add(t,strlen(p))) {
        #endif
                cntAdd++;
            } else {
                cntDup++;
            }
        }
    }
    
    fclose(fp);

    printf("\nLoad from file [%s]: Adding %d items, duplicated %d items.\n",
            FileName,cntAdd,cntDup);
    
    double density, avgDeeps;
    int maxDeeps;
    X.GetStatistics(density,avgDeeps,maxDeeps);
    printf("Density=%.2f, AvgDeeps=%.2f, MaxDeeps=%d.\n",
            density,avgDeeps,maxDeeps);
}

int main ( int argc, char *argv[] )
{
    struct timeval tv1;
    gettimeofday(&tv1,NULL);
{
    bool listflag = false;
    int nth = 1;
    if (argc > 1 && strcmp(argv[nth],"-l") == 0) {
        listflag = true;
        nth++;
    }
    int HashSize = argc > nth ? atoi(argv[nth]) : 5000;
    HashList X(HashSize);
    X.max_load_factor(1,8,20);

    if (nth+1 < argc) {
        while (++nth < argc) {
            Load(X,argv[nth]);
        }
    } else {
    #if defined(STRING_VER)
        string key("TEST");
        string val("abc");

        X.Add(key,"Hello World");
        if (X.Find(key,val)) {
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),val.c_str());

            X[key] = "xyz";
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),X[key].c_str());
            
            key = "OTHERS";
            printf("X.Find(\"%s\") = %s\n",key.c_str(),X.Find(key)?"true":"false");
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),X[key].c_str());
        }
    #endif
    }

    printf("\nHashSize=%zu, Total buckets=%zu.\n",
            X.HashSize(),X.Count());

    if (listflag) {
        printf("\n");
        for (int i=0; i < X.Count(); i++) {
            const char* key = X.Keys(i).c_str();
        #if defined(STRING_VER)
            const char* val = X.Values(i).c_str();
            printf("Key[%s] -> Value[%s]\n",key,val);
        #elif defined(CHARPTR_VER)
            const char* val = X.Values(i);
            printf("Key[%s] -> Value[%s]\n",key,val);
        #elif defined(INTEGER_VER)
            int val = X.Values(i);
            printf("Key[%s] -> Value[%d]\n",key,val);
        #endif
        }
    }

    int who = RUSAGE_SELF; 
    struct rusage usage; 
    getrusage(who,&usage);
    double mem;
    printf("\nusrtime=%.3fs, systime=%.3fs, rss=%.2fMB.\n",
            usage.ru_utime.tv_sec+usage.ru_utime.tv_usec/1000000.0,
            usage.ru_stime.tv_sec+usage.ru_stime.tv_usec/1000000.0,
            mem=usage.ru_maxrss/(1024*1024.0));
    if (mem == 0) MemUsage();
}
    struct timeval tv2;
    gettimeofday(&tv2,NULL);
    double elapsed = (tv2.tv_sec-tv1.tv_sec)+(tv2.tv_usec-tv1.tv_usec)/1000000.0;
    printf("Elapsed %.3fs\n",elapsed);

    printf("\n");

    return 0;
}
