/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Microbenchmark for LOCAL-PERF-001: syntax_property call overhead
   vs flat array lookup for ASCII characters.

   Measures the core per-call cost difference between:
     (A) unpatched: function call + pointer indirections (syntax_property path)
     (B) patched:   flat array load (SYNTAX_TRU fast path)

   No Emacs build required — simulates the call pattern in pure C.

   Compile:  cc -O2 -o bench_syntax_overhead bench_syntax_overhead.c
   Run:      ./bench_syntax_overhead
   Output:   timing table + raw JSON to stdout

   Override repetition counts at compile time:
     cc -O2 -DWARMUP_REPS=5 -DBENCH_REPS=20 -o bench bench_syntax_overhead.c */

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

#define ITERATIONS 100000000UL   /* 100 million */

/* ---- Minimal syntax simulation ---- */

typedef unsigned char bool_bf;

enum syntaxcode {
  Swhitespace = 0, Sword = 1, Spunct = 2,
  Sopen = 3, Sclose = 4, Smax = 16
};

/* Simulate the char-table structure: a pointer chain like Emacs's.
   Two levels of indirection to simulate the real overhead. */
struct char_table_entry {
  unsigned char code;
};

static struct char_table_entry g_char_table[128];
static struct char_table_entry *g_table_ptr = g_char_table;

/* Simulate syntax_property: function call + pointer dereference + bounds check. */
static __attribute__((noinline)) int
sim_syntax_property (int c, int ignore_prop)
{
  (void) ignore_prop;
  if ((unsigned) c >= 128) return Swhitespace;
  return g_table_ptr[c].code;
}

/* Unpatched path: always calls sim_syntax_property */
#define SYNTAX_SLOW(c) ((enum syntaxcode) sim_syntax_property((c), 0))

/* Patched struct */
struct re_pattern_buffer {
  unsigned char syntax_ascii[128];
  bool_bf syntax_ascii_valid;
};

/* Patched fast path */
#define SYNTAX_TRU(bufp, c) \
  (((bufp)->syntax_ascii_valid && (unsigned)(c) < 128) \
   ? (enum syntaxcode)(bufp)->syntax_ascii[(unsigned char)(c)] \
   : SYNTAX_SLOW(c))

/* ---- Timing ---- */

static double
clock_seconds (void)
{
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ---- Benchmark kernels ---- */

/* Unpatched: repeated syntax_property calls for ASCII chars. */
static volatile int g_sink;

static double
bench_slow_path (void)
{
  int acc = 0;
  double t0 = clock_seconds ();
  for (unsigned long i = 0; i < ITERATIONS; i++)
    {
      int c = (int)(i & 127);
      acc += (int) SYNTAX_SLOW (c);
    }
  double elapsed = clock_seconds () - t0;
  g_sink = acc;  /* prevent dead-code elimination */
  return elapsed;
}

/* Patched: flat array lookup. */
static double
bench_fast_path (struct re_pattern_buffer *bufp)
{
  int acc = 0;
  double t0 = clock_seconds ();
  for (unsigned long i = 0; i < ITERATIONS; i++)
    {
      int c = (int)(i & 127);
      acc += (int) SYNTAX_TRU (bufp, c);
    }
  double elapsed = clock_seconds () - t0;
  g_sink = acc;
  return elapsed;
}

/* ---- Statistics ---- */

static double
median (double *arr, int n)
{
  /* Simple insertion sort for small n */
  for (int i = 1; i < n; i++)
    {
      double key = arr[i];
      int j = i - 1;
      while (j >= 0 && arr[j] > key)
        { arr[j+1] = arr[j]; j--; }
      arr[j+1] = key;
    }
  return (n % 2) ? arr[n/2] : (arr[n/2-1] + arr[n/2]) / 2.0;
}

static double
mean (double *arr, int n)
{
  double s = 0;
  for (int i = 0; i < n; i++) s += arr[i];
  return s / n;
}

static double
stdev (double *arr, int n, double m)
{
  double s = 0;
  for (int i = 0; i < n; i++) s += (arr[i] - m) * (arr[i] - m);
  return (n > 1) ? sqrt(s / (n-1)) : 0.0;
}

/* ---- Main ---- */

int
main (void)
{
  /* Setup */
  for (int i = 0; i < 128; i++)
    {
      if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9'))
        g_char_table[i].code = Sword;
      else
        g_char_table[i].code = Swhitespace;
    }

  struct re_pattern_buffer bufp;
  memset (&bufp, 0, sizeof bufp);
  for (int i = 0; i < 128; i++)
    bufp.syntax_ascii[i] = g_char_table[i].code;
  bufp.syntax_ascii_valid = 1;

  /* Machine info */
  printf ("=== LOCAL-PERF-001 benchmark: syntax_property overhead ===\n");
  printf ("Host:      %s\n", "Linux (see /proc/cpuinfo for CPU details)");
  printf ("Warmup:    %d runs\n", WARMUP_REPS);
  printf ("Measured:  %d runs\n", BENCH_REPS);
  printf ("Iters/run: %lu\n\n", ITERATIONS);

  double slow_samples[BENCH_REPS];
  double fast_samples[BENCH_REPS];

  /* Warmup */
  printf ("Warming up...\n");
  for (int i = 0; i < WARMUP_REPS; i++)
    {
      bench_slow_path ();
      bench_fast_path (&bufp);
    }

  /* Measure slow path */
  printf ("Measuring unpatched (slow path)...\n");
  for (int i = 0; i < BENCH_REPS; i++)
    {
      slow_samples[i] = bench_slow_path ();
      printf ("  run %2d: %.4f s (%.2f ns/op)\n",
              i+1, slow_samples[i],
              slow_samples[i] * 1e9 / ITERATIONS);
    }

  /* Measure fast path */
  printf ("Measuring patched (fast path)...\n");
  for (int i = 0; i < BENCH_REPS; i++)
    {
      fast_samples[i] = bench_fast_path (&bufp);
      printf ("  run %2d: %.4f s (%.2f ns/op)\n",
              i+1, fast_samples[i],
              fast_samples[i] * 1e9 / ITERATIONS);
    }

  /* Statistics */
  double slow_median = median (slow_samples, BENCH_REPS);
  double fast_median = median (fast_samples, BENCH_REPS);
  double slow_mean   = mean (slow_samples, BENCH_REPS);
  double fast_mean   = mean (fast_samples, BENCH_REPS);
  double slow_sd     = stdev (slow_samples, BENCH_REPS, slow_mean);
  double fast_sd     = stdev (fast_samples, BENCH_REPS, fast_mean);
  double speedup     = slow_median / fast_median;

  printf ("\n=== Summary ===\n");
  printf ("%-30s %10s %10s %10s %10s\n",
          "Variant", "Median(s)", "Mean(s)", "Stdev(s)", "ns/op");
  printf ("%-30s %10.4f %10.4f %10.4f %10.2f\n",
          "unpatched (slow path)",
          slow_median, slow_mean, slow_sd,
          slow_median * 1e9 / ITERATIONS);
  printf ("%-30s %10.4f %10.4f %10.4f %10.2f\n",
          "patched (fast path)",
          fast_median, fast_mean, fast_sd,
          fast_median * 1e9 / ITERATIONS);
  printf ("Speedup (median): %.2fx\n\n", speedup);

  /* Machine-readable output */
  printf ("=== JSON ===\n");
  printf ("{\n");
  printf ("  \"issue\": \"LOCAL-PERF-001\",\n");
  printf ("  \"revision\": \"3ca168b80ae6d7b25fe55784dde3ad24faff7be2\",\n");
  printf ("  \"patch\": \"families/regex-syntax-cache/baseline/implementation.patch\",\n");
  printf ("  \"machine\": \"see uname -sr\",\n");
  printf ("  \"os\": \"Linux\",\n");
  printf ("  \"compiler\": \"cc (see cc --version)\",\n");
  printf ("  \"configure_flags\": [\"standalone-C-benchmark\"],\n");
  printf ("  \"iterations_per_run\": %lu,\n", ITERATIONS);
  printf ("  \"warmup_runs\": %d,\n", WARMUP_REPS);
  printf ("  \"measured_runs\": %d,\n", BENCH_REPS);
  printf ("  \"slow_path\": {\n");
  printf ("    \"samples\": [");
  for (int i = 0; i < BENCH_REPS; i++)
    printf ("%s%.6f", i ? ", " : "", slow_samples[i]);
  printf ("],\n");
  printf ("    \"median\": %.6f,\n", slow_median);
  printf ("    \"mean\": %.6f,\n", slow_mean);
  printf ("    \"stdev\": %.6f,\n", slow_sd);
  printf ("    \"units\": \"seconds\"\n");
  printf ("  },\n");
  printf ("  \"fast_path\": {\n");
  printf ("    \"samples\": [");
  for (int i = 0; i < BENCH_REPS; i++)
    printf ("%s%.6f", i ? ", " : "", fast_samples[i]);
  printf ("],\n");
  printf ("    \"median\": %.6f,\n", fast_median);
  printf ("    \"mean\": %.6f,\n", fast_mean);
  printf ("    \"stdev\": %.6f,\n", fast_sd);
  printf ("    \"units\": \"seconds\"\n");
  printf ("  },\n");
  printf ("  \"speedup_median\": %.4f\n", speedup);
  printf ("}\n");

  return 0;
}
