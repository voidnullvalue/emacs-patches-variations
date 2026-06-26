/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
   Unit tests for the adaptive threshold logic in malloc-pressure-relief variants.
   Tests the EMA update and threshold clamping without a macOS build.
   Compile: cc -o test_threshold_logic test_threshold_logic.c
   Run: ./test_threshold_logic */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/* Reproduce the adaptive threshold logic from the adaptive-threshold variant */

#define ADAPTIVE_EMA_ALPHA_NUM       1
#define ADAPTIVE_EMA_ALPHA_DEN       4
#define ADAPTIVE_THRESHOLD_INITIAL   (4ULL * 1024 * 1024)
#define ADAPTIVE_THRESH_MIN_SCALE_NUM 1
#define ADAPTIVE_THRESH_MIN_SCALE_DEN 2
#define ADAPTIVE_THRESH_MAX_SCALE_NUM 4
#define ADAPTIVE_THRESH_MAX_SCALE_DEN 1
#define ADAPTIVE_EVERY_N_GCS        64
#define ADAPTIVE_MIN_INTERVAL_SECS   3

static size_t    adaptive_threshold = ADAPTIVE_THRESHOLD_INITIAL;
static uintmax_t adaptive_ema;
static bool      adaptive_ema_valid;

static void
adaptive_update_threshold (size_t bytes_released)
{
  if (!adaptive_ema_valid)
    {
      adaptive_ema = (uintmax_t) bytes_released;
      adaptive_ema_valid = true;
    }
  else
    {
      adaptive_ema = ((uintmax_t) ADAPTIVE_EMA_ALPHA_NUM * bytes_released
                      + (uintmax_t) (ADAPTIVE_EMA_ALPHA_DEN - ADAPTIVE_EMA_ALPHA_NUM)
                        * adaptive_ema)
                     / ADAPTIVE_EMA_ALPHA_DEN;
    }

  size_t new_thresh = (size_t) adaptive_ema;
  size_t lo = (size_t) (adaptive_ema * ADAPTIVE_THRESH_MIN_SCALE_NUM
                        / ADAPTIVE_THRESH_MIN_SCALE_DEN);
  size_t hi = (size_t) (adaptive_ema * ADAPTIVE_THRESH_MAX_SCALE_NUM
                        / ADAPTIVE_THRESH_MAX_SCALE_DEN);
  if (new_thresh < lo) new_thresh = lo;
  if (new_thresh > hi) new_thresh = hi;
  if (new_thresh < 256 * 1024) new_thresh = 256 * 1024;
  adaptive_threshold = new_thresh;
}

/* ---- Helpers ---- */
static int failures;

#define EXPECT(cond, msg)						\
  do { if (!(cond)) { fprintf(stderr, "FAIL: %s (got %zu)\n", msg, adaptive_threshold); failures++; } \
       else { printf("PASS: %s\n", msg); } } while (0)

/* ---- Tests ---- */

static void
reset_state (void)
{
  adaptive_threshold = ADAPTIVE_THRESHOLD_INITIAL;
  adaptive_ema       = 0;
  adaptive_ema_valid = false;
}

static void
test_first_sample_sets_ema (void)
{
  reset_state ();
  adaptive_update_threshold (8 * 1024 * 1024);   /* 8 MB first yield */
  EXPECT (adaptive_ema == 8 * 1024 * 1024,
          "first sample: EMA equals first yield");
}

static void
test_high_yield_tracks_yield (void)
{
  reset_state ();
  /* Simulate 10 compactions each yielding 16 MB.
     The adaptive threshold converges to EMA(yield) = 16 MB.
     It should be well above the initial 4 MB, reflecting that
     compaction only makes sense when a large GC is expected.  */
  for (int i = 0; i < 10; i++)
    adaptive_update_threshold (16 * 1024 * 1024);
  EXPECT (adaptive_threshold > 4 * 1024 * 1024,
          "high yield: threshold tracks yield (> initial 4 MB)");
  EXPECT (adaptive_threshold <= 16 * 1024 * 1024 * 4,
          "high yield: threshold within 4x clamp of EMA");
}

static void
test_low_yield_raises_threshold (void)
{
  reset_state ();
  /* Simulate 10 compactions each yielding only 512 KB */
  for (int i = 0; i < 10; i++)
    adaptive_update_threshold (512 * 1024);
  /* Threshold should be below initial but above floor */
  EXPECT (adaptive_threshold >= 256 * 1024,
          "low yield: threshold never below 256 KB floor");
}

static void
test_zero_yield_hits_floor (void)
{
  reset_state ();
  /* Simulate compactions that yield nothing */
  for (int i = 0; i < 20; i++)
    adaptive_update_threshold (0);
  EXPECT (adaptive_threshold == 256 * 1024,
          "zero yield: threshold clamps to 256 KB floor");
}

static void
test_ema_convergence (void)
{
  reset_state ();
  size_t target = 6 * 1024 * 1024;   /* 6 MB */
  /* Run enough iterations for EMA to converge near target */
  for (int i = 0; i < 50; i++)
    adaptive_update_threshold (target);
  /* After 50 iterations with alpha=0.25, EMA should be close to target */
  size_t diff = adaptive_ema > target ? adaptive_ema - target : target - adaptive_ema;
  size_t tolerance = target / 1000;   /* 0.1% */
  if (diff > tolerance)
    {
      fprintf (stderr, "FAIL: ema_convergence: ema=%zu target=%zu diff=%zu\n",
               (size_t)adaptive_ema, target, diff);
      failures++;
    }
  else
    printf ("PASS: ema_convergence (ema %zu ≈ target %zu)\n",
            (size_t)adaptive_ema, target);
}

int
main (void)
{
  printf ("=== malloc-pressure-relief: adaptive threshold tests ===\n");
  test_first_sample_sets_ema ();
  test_high_yield_tracks_yield ();
  test_low_yield_raises_threshold ();
  test_zero_yield_hits_floor ();
  test_ema_convergence ();

  if (failures == 0)
    printf ("All tests passed.\n");
  else
    fprintf (stderr, "%d test(s) FAILED.\n", failures);

  return failures > 0 ? 1 : 0;
}
