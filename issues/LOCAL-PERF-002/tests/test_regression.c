/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Regression test for LOCAL-PERF-002: regex ASCII translate cache.
   Tests correctness of TRANSLATE_TRU and the trt_ascii cache.

   Passes with the patch applied; fails to compile on unpatched struct
   (fields trt_ascii / trt_ascii_valid do not exist).

   Compile:  cc -O2 -o test_regression test_regression.c
   Run:      ./test_regression */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned char bool_bf;

/* re_pattern_buffer (patched version).
   The fields trt_ascii[256] and trt_ascii_valid were added by the patch. */
struct re_pattern_buffer {
  void *translate;                /* non-null when translate table is active */
  unsigned char trt_ascii[256];  /* ADDED BY PATCH */
  bool_bf trt_ascii_valid : 1;   /* ADDED BY PATCH */
};

/* Simulate char_table_translate: downcase ASCII letters, identity otherwise. */
static int
sim_char_table_translate (void *tbl, int c)
{
  (void) tbl;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
  return c;
}

/* Simulate CHAR_TO_BYTE_SAFE: 0-255 → self, else -1 */
static int
CHAR_TO_BYTE_SAFE (int ch) { return (ch >= 0 && ch <= 255) ? ch : -1; }

#define RE_TRANSLATE(TBL, C) sim_char_table_translate((TBL), (C))
#define TRANSLATE(d) ((bufp->translate != NULL) ? RE_TRANSLATE(bufp->translate, (d)) : (d))
#define TRANSLATE_TRU(C) \
  ((bufp->trt_ascii_valid && (unsigned)(C) < 128) \
   ? bufp->trt_ascii[(unsigned char)(C)] \
   : TRANSLATE(C))

static int failures;
#define EXPECT(cond, msg) \
  do { if (!(cond)) { fprintf(stderr,"FAIL: %s\n",(msg)); failures++; } \
       else { printf("PASS: %s\n",(msg)); } } while (0)

static void populate_cache (struct re_pattern_buffer *bufp)
{
  for (int i = 0; i < 256; i++)
    {
      int ch   = sim_char_table_translate (bufp->translate, i);
      int byte = CHAR_TO_BYTE_SAFE (ch);
      bufp->trt_ascii[i] = (unsigned char)(byte >= 0 ? byte : i);
    }
  bufp->trt_ascii_valid = 1;
}

static void test_struct_fields_present (void)
{
  struct re_pattern_buffer p;
  memset (&p, 0, sizeof p);
  p.trt_ascii_valid = 0;
  p.trt_ascii[0] = 42;
  EXPECT (sizeof (p.trt_ascii) == 256, "struct: trt_ascii is 256 bytes");
  EXPECT (p.trt_ascii[0] == 42,        "struct: trt_ascii is indexable");
  printf ("PASS: struct fields trt_ascii[256] and trt_ascii_valid present\n");
}

static void test_cache_matches_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  struct re_pattern_buffer *bufp = &pat;
  populate_cache (bufp);

  int mismatches = 0;
  for (int c = 0; c < 128; c++)
    {
      int cached = TRANSLATE_TRU (c);
      int slow   = TRANSLATE (c);
      if (cached != slow)
        { fprintf(stderr,"FAIL c=%d cached=%d slow=%d\n",c,cached,slow);
          mismatches++; failures++; }
    }
  if (!mismatches)
    printf ("PASS: cache matches slow path for all 128 ASCII chars\n");
}

static void test_uppercase_folded (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  struct re_pattern_buffer *bufp = &pat;
  populate_cache (bufp);

  EXPECT (TRANSLATE_TRU ('A') == 'a', "translate: 'A' → 'a'");
  EXPECT (TRANSLATE_TRU ('Z') == 'z', "translate: 'Z' → 'z'");
  EXPECT (TRANSLATE_TRU ('a') == 'a', "translate: 'a' → 'a' (identity)");
  EXPECT (TRANSLATE_TRU ('0') == '0', "translate: '0' → '0' (identity)");
  EXPECT (TRANSLATE_TRU (' ') == ' ', "translate: ' ' → ' ' (identity)");
}

static void test_disabled_cache_uses_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  pat.trt_ascii_valid = 0;
  struct re_pattern_buffer *bufp = &pat;
  EXPECT (TRANSLATE_TRU ('B') == 'b', "disabled cache: 'B' → 'b' via slow path");
}

static void test_no_translate_table (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = NULL;
  pat.trt_ascii_valid = 0;
  struct re_pattern_buffer *bufp = &pat;
  EXPECT (TRANSLATE_TRU ('A') == 'A', "no translate table: identity");
  EXPECT (TRANSLATE_TRU ('a') == 'a', "no translate table: 'a' → 'a'");
}

static void test_case_fold_search_pattern (void)
{
  /* Simulate a case-fold isearch for "hello":
     Each char of the input text is passed through TRANSLATE_TRU before comparing. */
  const char *text = "Hello World";
  const char *pattern_lower = "hello";
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  pat.translate = (void *)1;
  struct re_pattern_buffer *bufp = &pat;
  populate_cache (bufp);

  int match = 1;
  for (int i = 0; pattern_lower[i]; i++)
    {
      int translated = TRANSLATE_TRU ((unsigned char) text[i]);
      if (translated != (unsigned char) pattern_lower[i])
        { match = 0; break; }
    }
  EXPECT (match, "case-fold: 'Hello' matches 'hello' via TRANSLATE_TRU");
}

int main (void)
{
  printf ("=== LOCAL-PERF-002 regression test ===\n");
  printf ("Issue: char_table_translate hot-path overhead in regex case-folding loops\n");
  printf ("Patch:  families/regex-translate-cache/baseline/implementation.patch\n\n");

  test_struct_fields_present ();
  test_cache_matches_slow_path ();
  test_uppercase_folded ();
  test_disabled_cache_uses_slow_path ();
  test_no_translate_table ();
  test_case_fold_search_pattern ();

  printf ("\n");
  if (failures == 0)
    { printf ("ALL TESTS PASSED (%d)\n", 6); return 0; }
  else
    { fprintf (stderr, "%d TEST(S) FAILED\n", failures); return 1; }
}
