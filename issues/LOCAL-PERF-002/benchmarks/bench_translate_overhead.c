/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Microbenchmark for LOCAL-PERF-002: char_table_translate call overhead
   vs flat array lookup for ASCII characters (case-folding path).

   Compile: cc -O2 -lm -DWARMUP_REPS=5 -DBENCH_REPS=10 \
              -o bench bench_translate_overhead.c
   Run:     ./bench */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/* Simulate the char-table indirection chain for char_table_translate.
   Real Emacs has two levels of indirection plus type dispatch. */
struct translate_entry { int translated_code; };
static struct translate_entry g_translate_table[256];
static struct translate_entry *g_tbl_ptr = g_translate_table;

static __attribute__((noinline)) int
sim_char_table_translate (void *tbl, int c)
{
  (void) tbl;
  if ((unsigned) c >= 256) return c;   /* out-of-range: identity */
  return g_tbl_ptr[c].translated_code;
}

#define TRANSLATE_SLOW(c) \
  (g_tbl_ptr != NULL ? sim_char_table_translate(g_tbl_ptr, (c)) : (c))

struct re_pattern_buffer {
  unsigned char trt_ascii[256];
  bool_bf trt_ascii_valid;
};

#define TRANSLATE_TRU(bufp, c) \
  (((bufp)->trt_ascii_valid && (unsigned)(c) < 128) \
   ? (int)(bufp)->trt_ascii[(unsigned char)(c)] \
   : TRANSLATE_SLOW(c))

static double clock_seconds (void)
{
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static volatile int g_sink;

static double bench_slow_path (void)
{
  int acc = 0;
  double t0 = clock_seconds ();
  for (unsigned long i = 0; i < ITERATIONS; i++)
    acc += TRANSLATE_SLOW ((int)(i & 127));
  double el = clock_seconds () - t0;
  g_sink = acc;
  return el;
}

static double bench_fast_path (struct re_pattern_buffer *bufp)
{
  int acc = 0;
  double t0 = clock_seconds ();
  for (unsigned long i = 0; i < ITERATIONS; i++)
    acc += TRANSLATE_TRU (bufp, (int)(i & 127));
  double el = clock_seconds () - t0;
  g_sink = acc;
  return el;
}

static double arr_median (double *a, int n)
{
  for (int i=1;i<n;i++){double k=a[i];int j=i-1;while(j>=0&&a[j]>k){a[j+1]=a[j];j--;}a[j+1]=k;}
  return (n%2)?a[n/2]:(a[n/2-1]+a[n/2])/2.0;
}
static double arr_mean (double *a, int n) { double s=0; for(int i=0;i<n;i++)s+=a[i]; return s/n; }
static double arr_sd (double *a, int n, double m) { double s=0; for(int i=0;i<n;i++)s+=(a[i]-m)*(a[i]-m); return n>1?sqrt(s/(n-1)):0; }

int main (void)
{
  /* Setup: downcase ASCII letters */
  for (int i = 0; i < 256; i++)
    g_translate_table[i].translated_code = (i>='A'&&i<='Z') ? i-'A'+'a' : i;

  struct re_pattern_buffer bufp;
  memset (&bufp, 0, sizeof bufp);
  for (int i = 0; i < 256; i++)
    {
      int ch = sim_char_table_translate (g_tbl_ptr, i);
      bufp.trt_ascii[i] = (unsigned char)((ch >= 0 && ch <= 255) ? ch : i);
    }
  bufp.trt_ascii_valid = 1;

  printf ("=== LOCAL-PERF-002 benchmark: char_table_translate overhead ===\n");
  printf ("Warmup: %d  Measured: %d  Iters/run: %lu\n\n",
          WARMUP_REPS, BENCH_REPS, ITERATIONS);

  double slow[BENCH_REPS], fast[BENCH_REPS];

  for (int i=0;i<WARMUP_REPS;i++) { bench_slow_path(); bench_fast_path(&bufp); }

  printf ("Unpatched (slow path):\n");
  for (int i=0;i<BENCH_REPS;i++)
    { slow[i]=bench_slow_path(); printf("  run %2d: %.4fs (%.2fns/op)\n",i+1,slow[i],slow[i]*1e9/ITERATIONS); }

  printf ("Patched (fast path):\n");
  for (int i=0;i<BENCH_REPS;i++)
    { fast[i]=bench_fast_path(&bufp); printf("  run %2d: %.4fs (%.2fns/op)\n",i+1,fast[i],fast[i]*1e9/ITERATIONS); }

  double sm=arr_median(slow,BENCH_REPS), fm=arr_median(fast,BENCH_REPS);
  double smean=arr_mean(slow,BENCH_REPS), fmean=arr_mean(fast,BENCH_REPS);
  double ssd=arr_sd(slow,BENCH_REPS,smean), fsd=arr_sd(fast,BENCH_REPS,fmean);

  printf ("\n=== Summary ===\n");
  printf ("%-30s %10.4f %10.4f %10.4f %10.2f\n","unpatched (slow path)",sm,smean,ssd,sm*1e9/ITERATIONS);
  printf ("%-30s %10.4f %10.4f %10.4f %10.2f\n","patched (fast path)",fm,fmean,fsd,fm*1e9/ITERATIONS);
  printf ("Speedup (median): %.2fx\n\n", sm/fm);

  printf ("=== JSON ===\n{\n");
  printf ("  \"issue\": \"LOCAL-PERF-002\",\n");
  printf ("  \"revision\": \"3ca168b80ae6d7b25fe55784dde3ad24faff7be2\",\n");
  printf ("  \"patch\": \"families/regex-translate-cache/baseline/implementation.patch\",\n");
  printf ("  \"iterations_per_run\": %lu,\n", ITERATIONS);
  printf ("  \"warmup_runs\": %d, \"measured_runs\": %d,\n", WARMUP_REPS, BENCH_REPS);
  printf ("  \"slow_path\": {\"samples\": [");
  for (int i=0;i<BENCH_REPS;i++) printf ("%s%.6f",i?", ":"",slow[i]);
  printf ("], \"median\": %.6f, \"mean\": %.6f, \"stdev\": %.6f, \"units\": \"seconds\"},\n", sm, smean, ssd);
  printf ("  \"fast_path\": {\"samples\": [");
  for (int i=0;i<BENCH_REPS;i++) printf ("%s%.6f",i?", ":"",fast[i]);
  printf ("], \"median\": %.6f, \"mean\": %.6f, \"stdev\": %.6f, \"units\": \"seconds\"},\n", fm, fmean, fsd);
  printf ("  \"speedup_median\": %.4f\n}\n", sm/fm);
  return 0;
}
