/* LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
   Regression test for LOCAL-PERF-003: combined regex syntax + translate caches.
   Tests that both SYNTAX_TRU and TRANSLATE_TRU work correctly in the same struct.

   Compile: cc -O2 -o test_regression test_regression.c
   Run:     ./test_regression */

#include <stdio.h>
#include <string.h>

typedef unsigned char bool_bf;
enum syntaxcode { Swhitespace=0, Sword=1, Spunct=2, Sopen=3, Sclose=4, Smax=16 };

/* re_pattern_buffer with BOTH caches (combined patch). */
struct re_pattern_buffer {
  void           *translate;
  unsigned char   trt_ascii[256];   /* translate cache */
  bool_bf         trt_ascii_valid : 1;
  unsigned char   syntax_ascii[128]; /* syntax cache */
  bool_bf         syntax_ascii_valid : 1;
};

static unsigned char g_syntax[128];
static int sim_syntax_prop (int c) { return (unsigned)c<128 ? g_syntax[c] : 0; }
static int sim_translate   (void *t, int c) { (void)t; return (c>='A'&&c<='Z')?c-'A'+'a':c; }

#define SYNTAX(c)       ((enum syntaxcode) sim_syntax_prop(c))
#define TRANSLATE(c)    ((bufp->translate != NULL) ? sim_translate(bufp->translate,(c)) : (c))
#define SYNTAX_TRU(c)   ((bufp->syntax_ascii_valid&&(unsigned)(c)<128) \
                         ? (enum syntaxcode)bufp->syntax_ascii[(unsigned char)(c)] : SYNTAX(c))
#define TRANSLATE_TRU(c) ((bufp->trt_ascii_valid&&(unsigned)(c)<128) \
                          ? (int)bufp->trt_ascii[(unsigned char)(c)] : TRANSLATE(c))

static int failures;
#define EXPECT(cond,msg) do{if(!(cond)){fprintf(stderr,"FAIL: %s\n",(msg));failures++;}else printf("PASS: %s\n",(msg));}while(0)

static void setup (struct re_pattern_buffer *p)
{
  memset (p, 0, sizeof *p);
  p->translate = (void*)1;
  for (int c='a';c<='z';c++) g_syntax[c]=Sword;
  for (int c='A';c<='Z';c++) g_syntax[c]=Sword;
  for (int c='0';c<='9';c++) g_syntax[c]=Sword;
  g_syntax[' ']=Swhitespace; g_syntax['(']=Sopen; g_syntax[')']=Sclose;

  for (int i=0;i<128;i++) p->syntax_ascii[i] = (unsigned char)sim_syntax_prop(i);
  p->syntax_ascii_valid = 1;
  for (int i=0;i<256;i++) {
    int ch = (i<256)?sim_translate(p->translate,i):i;
    p->trt_ascii[i] = (unsigned char)((ch>=0&&ch<=255)?ch:i);
  }
  p->trt_ascii_valid = 1;
}

static void test_struct_size (void)
{
  struct re_pattern_buffer p; memset(&p,0,sizeof p);
  EXPECT (sizeof(p.syntax_ascii)==128, "struct: syntax_ascii is 128 bytes");
  EXPECT (sizeof(p.trt_ascii)==256,    "struct: trt_ascii is 256 bytes");
  printf ("PASS: combined struct fields present\n");
}

static void test_both_caches_correct (void)
{
  struct re_pattern_buffer pat;
  struct re_pattern_buffer *bufp = &pat;
  setup (bufp);

  int syn_mismatches = 0, trt_mismatches = 0;
  for (int c=0;c<128;c++) {
    if (SYNTAX_TRU(c) != SYNTAX(c))  syn_mismatches++;
    if (TRANSLATE_TRU(c) != TRANSLATE(c)) trt_mismatches++;
  }
  EXPECT (!syn_mismatches, "both caches: SYNTAX_TRU matches SYNTAX for all ASCII");
  EXPECT (!trt_mismatches, "both caches: TRANSLATE_TRU matches TRANSLATE for all ASCII");
}

static void test_independent_validity (void)
{
  struct re_pattern_buffer pat;
  struct re_pattern_buffer *bufp = &pat;
  setup (bufp);

  /* Disable syntax cache, translate still works */
  bufp->syntax_ascii_valid = 0;
  EXPECT (TRANSLATE_TRU('A') == 'a', "independence: translate works when syntax invalid");
  EXPECT (SYNTAX_TRU('a') == Sword,  "independence: syntax slow path works when invalid");

  /* Re-enable syntax, disable translate */
  bufp->syntax_ascii_valid = 1;
  bufp->trt_ascii_valid = 0;
  EXPECT (SYNTAX_TRU('a') == Sword,  "independence: syntax works when translate invalid");
  EXPECT (TRANSLATE_TRU('A') == 'a', "independence: translate slow path works when invalid");
}

static void test_combined_word_boundary_and_fold (void)
{
  /* Simulated: case-fold isearch with word-boundary detection.
     Check that both TRANSLATE_TRU and SYNTAX_TRU give correct results together. */
  struct re_pattern_buffer pat;
  struct re_pattern_buffer *bufp = &pat;
  setup (bufp);

  const char *text = "Hello World";
  int i = 0;
  /* First char: 'H' should be a word char */
  EXPECT (SYNTAX_TRU((unsigned char)text[i]) == Sword, "combined: 'H' is Sword");
  EXPECT (TRANSLATE_TRU((unsigned char)text[i]) == 'h', "combined: 'H' folds to 'h'");
  /* Space: word boundary between 'o' and ' ' */
  EXPECT (SYNTAX_TRU(' ') == Swhitespace, "combined: ' ' is Swhitespace");
  EXPECT (TRANSLATE_TRU(' ') == ' ', "combined: ' ' is identity");
}

int main (void)
{
  printf ("=== LOCAL-PERF-003 regression test ===\n");
  printf ("Issue: combined regex syntax+translate caches\n");
  printf ("Patch: families/regex-combined-cache/baseline/implementation.patch\n\n");
  test_struct_size ();
  test_both_caches_correct ();
  test_independent_validity ();
  test_combined_word_boundary_and_fold ();
  printf ("\n");
  if (!failures) { printf ("ALL TESTS PASSED\n"); return 0; }
  else { fprintf (stderr, "%d TEST(S) FAILED\n", failures); return 1; }
}
