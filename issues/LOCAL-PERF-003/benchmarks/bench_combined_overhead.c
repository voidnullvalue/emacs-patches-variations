/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Microbenchmark for LOCAL-PERF-003: combined syntax+translate cache overhead.
   Measures the cumulative speedup from applying both caches together.
   Compile: cc -O2 -lm -DWARMUP_REPS=5 -DBENCH_REPS=10 -o bench bench_combined_overhead.c */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifndef WARMUP_REPS
#define WARMUP_REPS 5
#endif
#ifndef BENCH_REPS
#define BENCH_REPS 10
#endif
#define ITERATIONS 100000000UL

typedef unsigned char bool_bf;
enum syntaxcode { Swhitespace=0, Sword=1, Spunct=2, Smax=16 };

static unsigned char g_syntax[128];
struct translate_entry { int v; };
static struct translate_entry g_trt[256];

static __attribute__((noinline)) int sim_syntax (int c)
  { return (unsigned)c<128 ? g_syntax[c] : 0; }
static __attribute__((noinline)) int sim_translate (void *t, int c)
  { (void)t; return (unsigned)c<256 ? g_trt[c].v : c; }

struct re_pattern_buffer {
  void          *translate;
  unsigned char  trt_ascii[256];
  bool_bf        trt_ascii_valid : 1;
  unsigned char  syntax_ascii[128];
  bool_bf        syntax_ascii_valid : 1;
};

#define SYNTAX(c)        ((enum syntaxcode)sim_syntax(c))
#define TRANSLATE(c)     (p->translate ? sim_translate(p->translate,(c)) : (c))
#define SYNTAX_TRU(c)    ((p->syntax_ascii_valid&&(unsigned)(c)<128) \
                          ? (enum syntaxcode)p->syntax_ascii[(unsigned char)(c)] : SYNTAX(c))
#define TRANSLATE_TRU(c) ((p->trt_ascii_valid&&(unsigned)(c)<128) \
                          ? (int)p->trt_ascii[(unsigned char)(c)] : TRANSLATE(c))

static double clock_s (void)
  { struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }
static volatile int g_sink;

/* Unpatched: both paths go through function calls */
static double bench_both_slow (void)
{
  int acc=0;
  static struct re_pattern_buffer dummy_p;
  static int dummy_init;
  if (!dummy_init) {
    dummy_p.translate=(void*)1;
    dummy_p.syntax_ascii_valid=0;
    dummy_p.trt_ascii_valid=0;
    dummy_init=1;
  }
  struct re_pattern_buffer *p = &dummy_p;
  double t0=clock_s();
  for (unsigned long i=0;i<ITERATIONS;i++) {
    int c=(int)(i&127);
    acc += sim_syntax(c) + sim_translate(p->translate,c);
  }
  double el=clock_s()-t0; g_sink=acc; return el;
}

/* Patched: both caches active */
static double bench_both_fast (struct re_pattern_buffer *p)
{
  int acc=0;
  double t0=clock_s();
  for (unsigned long i=0;i<ITERATIONS;i++) {
    int c=(int)(i&127);
    acc += (int)SYNTAX_TRU(c) + TRANSLATE_TRU(c);
  }
  double el=clock_s()-t0; g_sink=acc; return el;
}

static double arr_med(double*a,int n)
  { for(int i=1;i<n;i++){double k=a[i];int j=i-1;while(j>=0&&a[j]>k){a[j+1]=a[j];j--;}a[j+1]=k;}
    return n%2?a[n/2]:(a[n/2-1]+a[n/2])/2.0; }
static double arr_mean(double*a,int n){double s=0;for(int i=0;i<n;i++)s+=a[i];return s/n;}
static double arr_sd(double*a,int n,double m){double s=0;for(int i=0;i<n;i++)s+=(a[i]-m)*(a[i]-m);return n>1?sqrt(s/(n-1)):0;}

int main (void)
{
  for (int i=0;i<128;i++) g_syntax[i]=(i>='a'&&i<='z')||(i>='A'&&i<='Z')||(i>='0'&&i<='9')?Sword:Swhitespace;
  for (int i=0;i<256;i++) g_trt[i].v=(i>='A'&&i<='Z')?i-'A'+'a':i;

  struct re_pattern_buffer pat; memset(&pat,0,sizeof pat);
  struct re_pattern_buffer *p = &pat;
  p->translate=(void*)1;
  for (int i=0;i<128;i++) p->syntax_ascii[i]=(unsigned char)sim_syntax(i);
  p->syntax_ascii_valid=1;
  for (int i=0;i<256;i++) p->trt_ascii[i]=(unsigned char)g_trt[i].v;
  p->trt_ascii_valid=1;

  printf ("=== LOCAL-PERF-003 benchmark: combined cache overhead ===\n");
  printf ("Warmup: %d  Measured: %d  Iters/run: %lu\n\n",WARMUP_REPS,BENCH_REPS,ITERATIONS);

  double slow[BENCH_REPS], fast[BENCH_REPS];
  for (int i=0;i<WARMUP_REPS;i++){bench_both_slow();bench_both_fast(p);}

  printf ("Both paths unpatched:\n");
  for (int i=0;i<BENCH_REPS;i++){slow[i]=bench_both_slow();printf("  run %2d: %.4fs (%.2fns/op)\n",i+1,slow[i],slow[i]*1e9/ITERATIONS);}
  printf ("Both paths patched:\n");
  for (int i=0;i<BENCH_REPS;i++){fast[i]=bench_both_fast(p);printf("  run %2d: %.4fs (%.2fns/op)\n",i+1,fast[i],fast[i]*1e9/ITERATIONS);}

  double sm=arr_med(slow,BENCH_REPS),fm=arr_med(fast,BENCH_REPS);
  double smean=arr_mean(slow,BENCH_REPS),fmean=arr_mean(fast,BENCH_REPS);
  double ssd=arr_sd(slow,BENCH_REPS,smean),fsd=arr_sd(fast,BENCH_REPS,fmean);
  printf ("\n=== Summary ===\n");
  printf ("%-30s %10.4f %10.2fns/op\n","both unpatched",sm,sm*1e9/ITERATIONS);
  printf ("%-30s %10.4f %10.2fns/op\n","both patched",fm,fm*1e9/ITERATIONS);
  printf ("Speedup (median): %.2fx\n\n",sm/fm);

  printf ("=== JSON ===\n{\n");
  printf ("  \"issue\": \"LOCAL-PERF-003\",\n");
  printf ("  \"revision\": \"3ca168b80ae6d7b25fe55784dde3ad24faff7be2\",\n");
  printf ("  \"patch\": \"families/regex-combined-cache/baseline/implementation.patch\",\n");
  printf ("  \"iterations_per_run\": %lu,\n",ITERATIONS);
  printf ("  \"warmup_runs\": %d, \"measured_runs\": %d,\n",WARMUP_REPS,BENCH_REPS);
  printf ("  \"slow_path\": {\"samples\": [");
  for(int i=0;i<BENCH_REPS;i++) printf("%s%.6f",i?", ":"",slow[i]);
  printf("], \"median\": %.6f, \"mean\": %.6f, \"stdev\": %.6f, \"units\": \"seconds\"},\n",sm,smean,ssd);
  printf ("  \"fast_path\": {\"samples\": [");
  for(int i=0;i<BENCH_REPS;i++) printf("%s%.6f",i?", ":"",fast[i]);
  printf("], \"median\": %.6f, \"mean\": %.6f, \"stdev\": %.6f, \"units\": \"seconds\"},\n",fm,fmean,fsd);
  printf ("  \"speedup_median\": %.4f\n}\n",sm/fm);
  return 0;
}
