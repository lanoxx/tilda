/*
 * This is an extremely simple converter for the Tilda-0.9.4 and less
 * key format to the new Tilda-0.9.5 key format, which is the standard
 * key format used throughout GNOME, as far as I can tell.
 *
 * When the program is done running, the converted key string, in the
 * new format, will be in the variable new_keystr.
 */

%{

/* Storage for the string-reading yyinput function */
static char *myinput;
static char *myinputptr;
static char *myinputeos;

/* My own string-reading yyinput function */
static int my_yyinput (char *buf, int maxsize);

#undef YY_INPUT
#define YY_INPUT(b,r,ms) (r = my_yyinput (b, ms))

/* This changes the name of the generated lexer to yylex_keystring */
#define YY_DECL int yylex_keystring (void)

#include <string.h>
#define append_to_keystring(S) strncat(new_keystr, (S), 63)

static char new_keystr[64] = "";
%}

CONTROL		([Cc][Oo][Nn][Tt][Rr][Oo][Ll]|[Cc][Nn][Tt][Rr][Ll]|[Cc][Tt][Rr][Ll]|[Cc][Tt][Ll])
SHIFT		([Ss][Hh][Ii][Ff][Tt]|[Ss][Hh][Ff][Tt])
ALT		([Aa][Ll][Tt])
NONE		([Nn][Oo][Nn][Ee])
SUPER		([Ss][Uu][Pp][Ee][Rr])
WIN		([Ww][Ii][Nn]|[Ww][Ii][Nn][Dd][Oo][Ww][Ss])
MOD1		([Mm][Oo][Dd]1)
MOD2		([Mm][Oo][Dd]2)
MOD3		([Mm][Oo][Dd]3)
MOD4		([Mm][Oo][Dd]4)
MOD5		([Mm][Oo][Dd]5)
HYPER		([Hh][Yy][Pp][Ee][Rr])
META		([Mm][Ee][Tt][Aa])
GRAVE		([`])

SEP		\+

%%

%noyywrap

{CONTROL}{SEP}		append_to_keystring ("<Control>");
{SHIFT}{SEP}		append_to_keystring ("<Shift>");
{ALT}{SEP}		append_to_keystring ("<Alt>");
{NONE}
{SUPER}			append_to_keystring ("<Super>");
{WIN}			append_to_keystring ("<Super>");
{MOD1}			append_to_keystring ("<Alt>");
{MOD2}			append_to_keystring ("<Mod2>");
{MOD3}			append_to_keystring ("<Mod3>");
{MOD4}			append_to_keystring ("<Mod4>");
{MOD5}			append_to_keystring ("<Mod5>");
{HYPER}			append_to_keystring ("<Hyper>");
{META}			append_to_keystring ("<Meta>");
{GRAVE}			append_to_keystring ("grave");


[ \t\n\r]		/* chomp any space */
.			append_to_keystring (yytext);

%%

#define min(X,Y) ((X)<(Y)?(X):(Y))

static int my_yyinput (char *buf, int maxsize)
{
	int n = min (maxsize, myinputeos-myinputptr);

	if (n)
	{
		memcpy (buf, myinputptr, n);
		myinputptr += n;
	}

	return n;
}

#ifdef TEST_KEY_CONVERTER_STANDALONE
int main (int argc, char *argv[])
{
	char test_input[] = "Ctrl+Alt+q";

	/* setup string input */
	myinput = test_input;
	myinputptr = myinput;
	myinputeos = myinput+strlen(myinput);

	yylex_keystring ();

	printf ("The converted key is: |%s|\n", new_keystr);

	return 0;
}
#else

/* This takes a char* and upgrades the key to the new format used by
 * >=tilda-0.9.5. It should handle pretty much every case.
 *
 * Don't forget to free() the returned string. */
char *upgrade_key_to_095 (const char *old_key)
{
	myinput = strdup (old_key);
	myinputptr = myinput;
	myinputeos = myinput+strlen(myinput);

	yylex_keystring ();

	return strdup (new_keystr);
}
#endif

