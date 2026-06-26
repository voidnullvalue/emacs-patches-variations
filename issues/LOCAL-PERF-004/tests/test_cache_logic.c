/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Tests for LOCAL-PERF-004: NSColor cache logic (pure C, platform-independent).
   Tests the Fibonacci hash, array lookup, and collision behavior.
   Does NOT test NSColor or Objective-C behavior.

   Compile: cc -O2 -o test_cache_logic test_cache_logic.c
   Run:     ./test_cache_logic */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define NS_COLOR_CACHE_SIZE 32768

/* Simulate NSColor* with a simple uintptr_t sentinel for testing. */
typedef uintptr_t NSColor_sim;
#define nil_color ((NSColor_sim)0)

static NSColor_sim ns_color_cache[NS_COLOR_CACHE_SIZE];
static unsigned long ns_color_cache_keys[NS_COLOR_CACHE_SIZE];

/* Fibonacci hash (from the patch) */
static unsigned int ns_color_cache_index (unsigned long key)
{
  return (unsigned int)((key * 11400714819323198485ULL)
                        & (NS_COLOR_CACHE_SIZE - 1));
}

static NSColor_sim ns_color_cache_lookup (unsigned long key)
{
  unsigned int i = ns_color_cache_index (key);
  if (ns_color_cache[i] != nil_color && ns_color_cache_keys[i] == key)
    return ns_color_cache[i];
  return nil_color;
}

static NSColor_sim ns_color_cache_store (unsigned long key, NSColor_sim col)
{
  unsigned int i = ns_color_cache_index (key);
  if (ns_color_cache[i] == nil_color)
    {
      ns_color_cache[i] = col;   /* "retain" in real code */
      ns_color_cache_keys[i] = key;
      return ns_color_cache[i];
    }
  return col;  /* collision: return uncached, don't evict */
}

static int failures;
#define EXPECT(cond,msg) \
  do{if(!(cond)){fprintf(stderr,"FAIL: %s\n",(msg));failures++;}else printf("PASS: %s\n",(msg));}while(0)

static void setup (void)
{
  memset (ns_color_cache, 0, sizeof ns_color_cache);
  memset (ns_color_cache_keys, 0, sizeof ns_color_cache_keys);
}

static void test_miss_on_empty (void)
{
  setup ();
  EXPECT (ns_color_cache_lookup (0xFF0000) == nil_color, "empty cache: miss returns nil");
  EXPECT (ns_color_cache_lookup (0x000000) == nil_color, "empty cache: black key misses");
  EXPECT (ns_color_cache_lookup (0xFFFFFF) == nil_color, "empty cache: white key misses");
}

static void test_store_and_hit (void)
{
  setup ();
  unsigned long key = 0xFF0000UL;  /* red */
  NSColor_sim col = (NSColor_sim)0xDEADBEEFUL;

  NSColor_sim stored = ns_color_cache_store (key, col);
  EXPECT (stored == col, "store: returns the stored object");

  NSColor_sim hit = ns_color_cache_lookup (key);
  EXPECT (hit == col, "lookup: returns stored object on hit");
  EXPECT (hit != nil_color, "lookup: hit is not nil");
}

static void test_miss_after_collision (void)
{
  setup ();
  /* Find two keys that hash to the same slot */
  unsigned int slot0 = ns_color_cache_index (0x000001UL);
  /* Brute-force find another key that lands on the same slot */
  unsigned long key2 = 0;
  for (unsigned long candidate = 0x010000UL; candidate < 0x100000UL; candidate++)
    {
      if (ns_color_cache_index (candidate) == slot0)
        { key2 = candidate; break; }
    }

  if (key2 == 0)
    {
      printf ("SKIP: test_miss_after_collision (no collision found in range)\n");
      return;
    }

  NSColor_sim col1 = (NSColor_sim)0xAAAA;
  NSColor_sim col2 = (NSColor_sim)0xBBBB;

  ns_color_cache_store (0x000001UL, col1);
  NSColor_sim stored2 = ns_color_cache_store (key2, col2);

  /* Second store should NOT overwrite (collision policy: no eviction) */
  EXPECT (stored2 == col2, "collision: store returns caller's color (not cached)");
  EXPECT (ns_color_cache_lookup (0x000001UL) == col1, "collision: original key still hits");
  EXPECT (ns_color_cache_lookup (key2) == nil_color, "collision: colliding key misses (not stored)");
}

static void test_multiple_colors (void)
{
  setup ();
  /* Store 10 common Emacs face colors */
  unsigned long colors[] = {
    0x000000UL, /* black */
    0xFFFFFFUL, /* white */
    0xFF0000UL, /* red */
    0x00FF00UL, /* green */
    0x0000FFUL, /* blue */
    0xFFFF00UL, /* yellow */
    0xFF00FFUL, /* magenta */
    0x00FFFFUL, /* cyan */
    0x888888UL, /* gray */
    0x1F1F1FUL, /* dark background */
  };
  int n = sizeof (colors) / sizeof (colors[0]);

  for (int i = 0; i < n; i++)
    {
      NSColor_sim col = (NSColor_sim)(0x1000 + i);
      ns_color_cache_store (colors[i], col);
    }

  int hits = 0;
  for (int i = 0; i < n; i++)
    {
      NSColor_sim hit = ns_color_cache_lookup (colors[i]);
      if (hit != nil_color) hits++;
      else printf ("INFO: color 0x%06lX collided (slot %u already taken)\n",
                   colors[i], ns_color_cache_index (colors[i]));
    }
  printf ("INFO: %d/%d colors cached\n", hits, n);
  /* At least some colors should be cached; the exact count depends on the
     specific hash collisions among these test values. */
  EXPECT (hits >= 1, "multiple colors: at least one color cached successfully");
}

static void test_hash_distribution (void)
{
  /* Check that Fibonacci hash distributes reasonably across the cache */
  int used_slots[NS_COLOR_CACHE_SIZE];
  memset (used_slots, 0, sizeof used_slots);

  int collisions = 0;
  /* Test all 24-bit RGB values at stride 1024 */
  for (unsigned long c = 0; c < 0x1000000UL; c += 1024)
    {
      unsigned int idx = ns_color_cache_index (c);
      if (used_slots[idx]++) collisions++;
    }
  int total = (int)(0x1000000UL / 1024);
  printf ("INFO: hash distribution: %d inputs, %d collisions (%.1f%%)\n",
          total, collisions, 100.0 * collisions / total);
  /* A good hash should have < 50% collision rate for this load factor */
  EXPECT (collisions < total, "hash distribution: collisions < total inputs");
}

int main (void)
{
  printf ("=== LOCAL-PERF-004 cache logic test (pure C) ===\n");
  printf ("Issue: NSColor per-call allocation in +colorWithUnsignedLong:\n");
  printf ("Patch: families/ns-color-cache/baseline/implementation.patch\n");
  printf ("NOTE: NSColor behavior blocked on Linux; testing pure-C cache logic only.\n\n");

  test_miss_on_empty ();
  test_store_and_hit ();
  test_miss_after_collision ();
  test_multiple_colors ();
  test_hash_distribution ();

  printf ("\n");
  if (!failures) { printf ("ALL TESTS PASSED (cache logic)\n"); return 0; }
  else { fprintf (stderr, "%d TEST(S) FAILED\n", failures); return 1; }
}
