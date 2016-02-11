#ifndef _UTF_H_
#define _UTF_H_ 1
#if defined(__cplusplus)
extern "C" { 
#endif

typedef unsigned int Rune;	/* 32 bits */

enum
{
	UTFmax		= 4,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
	Runemax		= 0x10FFFF	/* maximum rune value */
};

int	chartorune(Rune *rune, char *str);
Rune*	runestrchr(Rune *s, Rune c);
char*	utfrune(char *s, Rune c);

#if defined(__cplusplus)
}
#endif
#endif
