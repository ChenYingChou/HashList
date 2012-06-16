// hash.cc
// vim: set ts=4 sw=4 et:

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <boost/unordered_map.hpp>
#include <string>

using namespace std;
using namespace boost;

#include <stdarg.h>

//------------------------------------------------------------------------------
namespace hashx
{
    template <std::size_t FnvPrime, std::size_t OffsetBasis>
    struct basic_fnv_1
    {
        std::size_t operator()(std::string const& text) const
        {
            std::size_t hash = OffsetBasis;
            for(std::string::const_iterator it = text.begin(), end = text.end();
                    it != end; ++it)
            {
                hash *= FnvPrime;
                hash ^= *it;
            }

            return hash;
        }
    };

    template <std::size_t FnvPrime, std::size_t OffsetBasis>
    struct basic_fnv_1a
    {
        std::size_t operator()(std::string const& text) const
        {
            std::size_t hash = OffsetBasis;
            for(std::string::const_iterator it = text.begin(), end = text.end();
                    it != end; ++it)
            {
                hash ^= *it;
                hash *= FnvPrime;
            }

            return hash;
        }
    };

    // For 32 bit machines:
    const std::size_t fnv_prime = 16777619u;
    const std::size_t fnv_offset_basis = 2166136261u;

    // For 64 bit machines:
    // const std::size_t fnv_prime = 1099511628211u;
    // const std::size_t fnv_offset_basis = 14695981039346656037u;

    // For 128 bit machines:
    // const std::size_t fnv_prime = 309485009821345068724781401u;
    // const std::size_t fnv_offset_basis = 275519064689413815358837431229664493455u;

    // For 256 bit machines:
    // const std::size_t fnv_prime = 374144419156711147060143317175368453031918731002211u;
    // const std::size_t fnv_offset_basis = 100029257958052580907070968620625704837092796014241193945225284501741471925557u;

    typedef basic_fnv_1<fnv_prime, fnv_offset_basis> fnv_1;
    typedef basic_fnv_1a<fnv_prime, fnv_offset_basis> fnv_1a;
}
//------------------------------------------------------------------------------
typedef boost::unordered_map<string, string, hashx::fnv_1a> HashList;

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
            
            string key(t);
            if (X.find(key) == X.end()) {
                X[key] = p;
                cntAdd++;
            } else {
                cntDup++;
            }
        }
    }
    
    fclose(fp);

    printf("\nLoad from file [%s]: Adding %d items, duplicated %d items.\n",
            FileName,cntAdd,cntDup);
    
    printf("load_factor=%.2f, max_load_factor=%.2f\n",
            X.load_factor(),X.max_load_factor());
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

    if (nth+1 < argc) {
        while (++nth < argc) {
            Load(X,argv[nth]);
        }
    } else {
        string key("TEST");
        string val("abc");

        X[key] = "Hello World";
        if (X.find(key) != X.end()) {
            val = X[key];
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),val.c_str());

            X[key] = "xyz";
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),X[key].c_str());
            
            key = "OTHERS";
            printf("X.Find(\"%s\") = %s\n",key.c_str(),X.find(key)!=X.end()?"true":"false");
            printf("X[\"%s\"] = \"%s\"\n",key.c_str(),X[key].c_str());
        }
    }

    printf("\nHashSize=%zu, Total buckets=%zu.\n",
            X.bucket_count(),X.size());

/*
    if (listflag) {
        printf("\n");
        for (int i=0; i < X.Count(); i++) {
            const char* key = X.Keys(i).c_str();
            const char* val = X.Values(i).c_str();
            printf("Key[%s] -> Value[%s]\n",key,val);
        }
    }
*/
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

