/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
   Unit tests for the regex ASCII translate cache (trt_ascii array).
   Tests the cache logic and correctness of CHAR_TO_BYTE_SAFE approximation.
   Compile: cc -o test_translate_cache test_translate_cache.c
   Run: ./test_translate_cache */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned char bool_bf;

/* Simulated translate table: downcase ASCII letters */
static int test_translate[256];

static int
char_table_translate_stub (void *tbl, int c)
{
  (void) tbl;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';   /* uppercase → lowercase */
  return c;
}

/* CHAR_TO_BYTE_SAFE stub: for codes 0-255, return the code.
   For codes > 255, return -1.  */
static int
CHAR_TO_BYTE_SAFE (int ch)
{
  return (ch >= 0 && ch <= 255) ? ch : -1;
}

/* Minimal re_pattern_buffer fields for translate cache */
struct re_pattern_buffer {
  void         *translate;    /* non-null if translate table is active */
  unsigned char trt_ascii[256];
  bool_bf       trt_ascii_valid;
};

#define RE_TRANSLATE(TBL, C) char_table_translate_stub((TBL), (C))
#define TRANSLATE(d)  ((bufp->translate != NULL) ? RE_TRANSLATE(bufp->translate, (d)) : (d))

#define TRANSLATE_TRU(C)					\
  ((bufp->trt_ascii_valid && (unsigned)(C) < 128)		\
   ? bufp->trt_ascii[(unsigned char)(C)]			\
   : TRANSLATE(C))

/* ---- Helpers ---- */
static int failures;

#define EXPECT(cond, msg)						\
  do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
       else { printf("PASS: %s\n", msg); } } while (0)

static void
populate_trt_cache (struct re_pattern_buffer *bufp)
{
  for (int i = 0; i < 256; i++)
    {
      int ch = char_table_translate_stub (bufp->translate, i);
      int byte = CHAR_TO_BYTE_SAFE (ch);
      bufp->trt_ascii[i] = (byte >= 0 ? byte : i);
    }
  bufp->trt_ascii_valid = 1;
}

/* ---- Tests ---- */

static void
test_trt_cache_matches_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;   /* non-null sentinel */
  struct re_pattern_buffer *bufp = &pat;

  populate_trt_cache (bufp);

  for (int c = 0; c < 128; c++)
    {
      int cached = TRANSLATE_TRU (c);
      int slow   = TRANSLATE (c);
      if (cached != slow)
        {
          fprintf (stderr,
                   "FAIL: trt_cache_matches_slow_path: c=%d cached=%d slow=%d\n",
                   c, cached, slow);
          failures++;
          return;
        }
    }
  printf ("PASS: trt_cache_matches_slow_path (all 128 ASCII chars)\n");
}

static void
test_trt_uppercase_folded (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  struct re_pattern_buffer *bufp = &pat;

  populate_trt_cache (bufp);

  EXPECT (TRANSLATE_TRU ('A') == 'a', "translate: 'A' folds to 'a'");
  EXPECT (TRANSLATE_TRU ('Z') == 'z', "translate: 'Z' folds to 'z'");
  EXPECT (TRANSLATE_TRU ('a') == 'a', "translate: 'a' maps to itself");
  EXPECT (TRANSLATE_TRU ('0') == '0', "translate: '0' maps to itself");
}

static void
test_trt_disabled_uses_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  pat.trt_ascii_valid = 0;   /* cache not valid */
  struct re_pattern_buffer *bufp = &pat;

  int r = TRANSLATE_TRU ('B');
  EXPECT (r == 'b', "trt disabled: 'B' folds to 'b' via slow path");
}

static void
test_trt_no_translate_table (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = NULL;   /* no translate table */
  pat.trt_ascii_valid = 0;
  struct re_pattern_buffer *bufp = &pat;

  int r = TRANSLATE_TRU ('A');
  EXPECT (r == 'A', "no translate table: 'A' maps to itself");
}

int
main (void)
{
  printf ("=== regex-translate-cache: logic tests ===\n");
  test_trt_cache_matches_slow_path ();
  test_trt_uppercase_folded ();
  test_trt_disabled_uses_slow_path ();
  test_trt_no_translate_table ();

  if (failures == 0)
    printf ("All tests passed.\n");
  else
    fprintf (stderr, "%d test(s) FAILED.\n", failures);

  return failures > 0 ? 1 : 0;
}
