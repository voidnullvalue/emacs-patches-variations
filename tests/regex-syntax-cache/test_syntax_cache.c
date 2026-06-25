/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
   Unit tests for the regex ASCII syntax cache (syntax_ascii array).
   Tests the cache logic in isolation without a full Emacs build.
   Compile: cc -o test_syntax_cache test_syntax_cache.c
   Run: ./test_syntax_cache */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/* Minimal stubs for types used in regex-emacs.h */
typedef unsigned char bool_bf;

/* Stub for syntaxcode enum — enough codes for testing */
enum syntaxcode {
  Swhitespace = 0,
  Sword        = 1,
  Spunct       = 2,
  Sopen        = 3,
  Sclose       = 4,
  Scomment     = 11,
  Sendcomment  = 12,
  Smax         = 16
};

/* Minimal re_pattern_buffer with only the fields we test */
struct re_pattern_buffer {
  unsigned char syntax_ascii[128];
  bool_bf       syntax_ascii_valid;
};

/* Simulate syntax_property for testing: use a simple table */
static unsigned char test_syntax_table[128];

static int
test_syntax_property (int c, int ignore_prop)
{
  (void) ignore_prop;
  if (c < 0 || c >= 128) return Swhitespace;
  return test_syntax_table[c];
}

/* Simulate SYNTAX macro (uses test_syntax_property) */
#define SYNTAX(c) ((enum syntaxcode) test_syntax_property((c), 0))

/* The macro under test */
#define SYNTAX_TRU(c)						\
  ((bufp->syntax_ascii_valid && (unsigned)(c) < 128)		\
   ? (enum syntaxcode) bufp->syntax_ascii[(unsigned char)(c)]	\
   : SYNTAX(c))

/* ---- Helpers ---- */
static int failures;

#define EXPECT(cond, msg)						\
  do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
       else { printf("PASS: %s\n", msg); } } while (0)

static void
populate_cache (struct re_pattern_buffer *bufp)
{
  for (int i = 0; i < 128; i++)
    bufp->syntax_ascii[i] = test_syntax_property (i, 0);
  bufp->syntax_ascii_valid = 1;
}

/* ---- Tests ---- */

static void
test_cache_hit_matches_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  struct re_pattern_buffer *bufp = &pat;

  /* Set up a non-trivial syntax table */
  for (int i = 'a'; i <= 'z'; i++) test_syntax_table[i] = Sword;
  for (int i = 'A'; i <= 'Z'; i++) test_syntax_table[i] = Sword;
  for (int i = '0'; i <= '9'; i++) test_syntax_table[i] = Sword;
  test_syntax_table[' ']  = Swhitespace;
  test_syntax_table['\t'] = Swhitespace;
  test_syntax_table['(']  = Sopen;
  test_syntax_table[')']  = Sclose;
  test_syntax_table[';']  = Scomment;

  populate_cache (bufp);

  for (int c = 0; c < 128; c++)
    {
      enum syntaxcode cached = SYNTAX_TRU (c);
      enum syntaxcode slow   = SYNTAX (c);
      if (cached != slow)
        {
          fprintf (stderr,
                   "FAIL: cache_hit_matches_slow_path: c=%d cached=%d slow=%d\n",
                   c, (int)cached, (int)slow);
          failures++;
          return;
        }
    }
  printf ("PASS: cache_hit_matches_slow_path (all 128 ASCII chars)\n");
}

static void
test_cache_disabled_uses_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  struct re_pattern_buffer *bufp = &pat;

  /* cache is NOT populated; syntax_ascii_valid = 0 */
  test_syntax_table['a'] = Sword;
  test_syntax_table[' '] = Swhitespace;

  /* SYNTAX_TRU should fall back to SYNTAX() */
  enum syntaxcode r_a = SYNTAX_TRU ('a');
  enum syntaxcode r_sp = SYNTAX_TRU (' ');

  EXPECT (r_a  == Sword,       "disabled cache: 'a' returns Sword via slow path");
  EXPECT (r_sp == Swhitespace, "disabled cache: ' ' returns Swhitespace via slow path");
}

static void
test_non_ascii_always_slow_path (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  struct re_pattern_buffer *bufp = &pat;

  populate_cache (bufp);

  /* Characters with code >= 128 should use SYNTAX(), not the cache.
     We can only test this structurally: for c >= 128, the macro should
     take the else branch (syntax_ascii_valid is true but (unsigned)(c) >= 128).  */
  int c = 200;   /* >= 128 */
  /* This would call SYNTAX(200) which calls test_syntax_property(200, 0).
     test_syntax_property clamps to Swhitespace for c >= 128.  */
  enum syntaxcode r = SYNTAX_TRU (c);
  EXPECT (r == Swhitespace, "non-ASCII (>=128) takes slow path");
}

static void
test_cache_invalidation_model (void)
{
  struct re_pattern_buffer pat;
  memset (&pat, 0, sizeof pat);
  struct re_pattern_buffer *bufp = &pat;

  test_syntax_table['x'] = Sword;
  populate_cache (bufp);

  EXPECT (SYNTAX_TRU ('x') == Sword, "invalidation: 'x' is Sword before change");

  /* Simulate syntax table change: invalidate by clearing the flag */
  bufp->syntax_ascii_valid = 0;
  test_syntax_table['x'] = Spunct;   /* Table changed */

  EXPECT (SYNTAX_TRU ('x') == Spunct,
          "invalidation: after flag cleared, slow path returns new value");

  /* Re-populate with new table */
  populate_cache (bufp);
  EXPECT (SYNTAX_TRU ('x') == Spunct,
          "invalidation: after re-populate, cache returns new value");
}

int
main (void)
{
  printf ("=== regex-syntax-cache: logic tests ===\n");
  test_cache_hit_matches_slow_path ();
  test_cache_disabled_uses_slow_path ();
  test_non_ascii_always_slow_path ();
  test_cache_invalidation_model ();

  if (failures == 0)
    printf ("All tests passed.\n");
  else
    fprintf (stderr, "%d test(s) FAILED.\n", failures);

  return failures > 0 ? 1 : 0;
}
