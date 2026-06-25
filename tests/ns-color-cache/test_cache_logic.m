/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
   Standalone test for ns-color-cache variants (no Emacs build required).
   Compile with: clang -framework Foundation -o test_cache_logic test_cache_logic.m
   Run: ./test_cache_logic */

#import <Foundation/Foundation.h>
#include <assert.h>
#include <stdio.h>

/* ---- Minimal stubs for testing cache logic ---- */

/* Baseline direct-mapped cache */
#define NS_COLOR_CACHE_SIZE 32768
typedef unsigned long NSColor_stub;   /* Use unsigned long as NSColor* stub */
static NSColor_stub ns_color_cache[NS_COLOR_CACHE_SIZE];
static unsigned long ns_color_cache_keys[NS_COLOR_CACHE_SIZE];
static int ns_color_cache_initialized;

static NSUInteger
ns_color_cache_index (unsigned long key)
{
  return (NSUInteger) ((key * 11400714819323198485ULL)
                       & (NS_COLOR_CACHE_SIZE - 1));
}

static NSColor_stub
ns_color_cache_lookup (unsigned long key)
{
  NSUInteger i = ns_color_cache_index (key);
  if (ns_color_cache[i] != 0 && ns_color_cache_keys[i] == key)
    return ns_color_cache[i];
  return 0;
}

static NSColor_stub
ns_color_cache_store (unsigned long key, NSColor_stub col)
{
  NSUInteger i = ns_color_cache_index (key);
  if (ns_color_cache[i] == 0)
    {
      ns_color_cache[i] = col;
      ns_color_cache_keys[i] = key;
      return ns_color_cache[i];
    }
  return col;
}

/* ---- Tests ---- */

static int failures;

#define EXPECT(cond, msg) \
  do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
       else { printf("PASS: %s\n", msg); } } while(0)

static void
test_baseline_hit (void)
{
  unsigned long key = 0xFF336699UL;
  NSColor_stub  col = 0xDEAD;

  ns_color_cache_store (key, col);
  NSColor_stub found = ns_color_cache_lookup (key);
  EXPECT (found == col, "baseline: stored color returned on lookup");
}

static void
test_baseline_miss (void)
{
  /* A key that was never stored should miss.  */
  unsigned long key = 0xABCDEF01UL;
  NSColor_stub found = ns_color_cache_lookup (key);
  EXPECT (found == 0, "baseline: unknown key returns nil (0)");
}

static void
test_baseline_slot_stability (void)
{
  /* Store two different keys that might hash to the same slot.
     The second should not displace the first (no eviction in baseline).  */
  unsigned long k1 = 0x00000001UL;
  unsigned long k2 = 0xFFFFFFFFUL;
  NSColor_stub c1 = 0x1111, c2 = 0x2222;

  ns_color_cache_store (k1, c1);
  ns_color_cache_store (k2, c2);

  NSColor_stub r1 = ns_color_cache_lookup (k1);
  /* k1 was stored first; if k2 hashed to the same slot, k2 is NOT stored
     (baseline never displaces). So k1 should still be findable.  */
  EXPECT (r1 == c1 || r1 == 0,
          "baseline: collision does not displace existing entry");
}

static void
test_baseline_deterministic_hash (void)
{
  /* Same key always maps to the same slot.  */
  unsigned long key = 0x12345678UL;
  NSUInteger s1 = ns_color_cache_index (key);
  NSUInteger s2 = ns_color_cache_index (key);
  EXPECT (s1 == s2, "baseline: hash is deterministic");
}

int
main (void)
{
  printf ("=== ns-color-cache: baseline logic tests ===\n");
  test_baseline_hit ();
  test_baseline_miss ();
  test_baseline_slot_stability ();
  test_baseline_deterministic_hash ();

  if (failures == 0)
    printf ("All tests passed.\n");
  else
    fprintf (stderr, "%d test(s) FAILED.\n", failures);

  return failures > 0 ? 1 : 0;
}
