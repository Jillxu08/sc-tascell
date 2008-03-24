/*-
 * Copyright (c) 1998, 2002-2004 Kiyoshi Matsui <kmatsui@t3.rim.or.jp>
 * All rights reserved.
 *
 * Some parts of this code are derived from the public domain software
 * DECUS cpp (1984,1985) written by Martin Minow.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *                          M B C H A R . C
 *      C h a r a c t e r    h a n d l i n g    R o u t i n e s
 */

/*
 * MCPP Version 2.4
 * 2004/02      kmatsui
 *      Created.  Moved type[] from system.c.
 *      Created type_*[] tables for various encodings of multi-byte character.
 *      Created encoding_name[][] table.
 *      Created mb_init(), mb_read_2byte(), mb_read_iso2022_jp(),
 *          mb_read_utf8(), mb_eval(), set_encoding(), search_encoding() and
 *          strip_bar().
 */

/*
 * Character handling and multi-byte character handling routines are
 * placed here.
 */

#if PREPROCESSED
#include    "cpp.H"
#else
#include    "system.H"
#include    "internal.H"
#endif

/*
 * Tables of character types and multi-byte character types.
 * These tables must be rewritten for a non-Ascii machine.
 */

#if DOLLAR_IN_NAME
#define DOL     LET
#else
#define DOL     000
#endif

#if SYSTEM == SYS_MSDOS
/* For MS-DOS or other 16-bit systems.  */

#define MB1     0x40            /* The first byte of MBCHAR     */
#define MB2     ((char)0x80)    /* The second byte of MBCHAR    */

#if MBCHAR
#define MB12    (MB1 | MB2)
#define LET2    (LET | MB2)
#define PNC2    (PUNC| MB2)
#endif

const char      type[ UCHARMAX + 1] = {

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   SPA,   SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
   000,   000,   000,   000,   LET,   000,   000,   SPA,    /* 18           */
   SPA,   PUNC,  QUO,   PUNC,  DOL,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

#if MBCHAR == SJIS || MBCHAR == BIGFIVE
   MB2,   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 40 @ABCDEFG  */
   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 48 HIJKLMNO  */
   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 50 PQRSTUVW  */
   LET2,  LET2,  LET2,  PNC2,  MB2,   PNC2,  PNC2,  LET2,   /* 58 XYZ[\]^_  */
   MB2,   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 60 `abcdefg  */
   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 68 hijklmno  */
   LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,  LET2,   /* 70 pqrstuvw  */
   LET2,  LET2,  LET2,  PNC2,  PNC2,  PNC2,  PNC2,  000,    /* 78 xyz{|}~   */
#else
/* MBCHAR == EUC_JP || MBCHAR == KSC5601 || MBCHAR == GB2312 || MBCHAR == 0 */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 40 @ABCDEFG  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 48 HIJKLMNO  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 50 PQRSTUVW  */
   LET,   LET,   LET,   PUNC,  000,   PUNC,  PUNC,  LET,    /* 58 XYZ[\]^_  */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 60 `abcdefg  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 68 hijklmno  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 70 pqrstuvw  */
   LET,   LET,   LET,   PUNC,  PUNC,  PUNC,  PUNC,  000,    /* 78 xyz{|}~   */
#endif

#if MBCHAR == SJIS
   MB2,   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   80 .. 87   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   88 .. 8F   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   90 .. 97   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   98 .. 9F   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   A0 .. A7   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   A8 .. AF   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   B0 .. B7   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   B8 .. BF   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   C0 .. C7   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   C8 .. CF   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   D0 .. D7   */
   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,   MB2,    /*   D8 .. DF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E0 .. E7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E8 .. EF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   F0 .. F7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  000,   000,   000,    /*   F8 .. FF   */
#else
#if MBCHAR == EUC_JP
   000,   000,   000,   000,   000,   000,   000,   000,    /*   80 .. 87   */
   000,   000,   000,   000,   000,   000,   MB1,   000,    /*   88 .. 8F   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   90 .. 97   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   98 .. 9F   */
   000,   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   A0 .. A7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   A8 .. AF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   B0 .. B7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   B8 .. BF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   C0 .. C7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   C8 .. CF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   D0 .. D7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   D8 .. DF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E0 .. E7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E8 .. EF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   F0 .. F7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  000,    /*   F8 .. FF   */
#else
#if MBCHAR == KSC5601 || MBCHAR == GB2312 || MBCHAR == BIGFIVE
   000,   000,   000,   000,   000,   000,   000,   000,    /*   80 .. 87   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   88 .. 8F   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   90 .. 97   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   98 .. 9F   */
   000,   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   A0 .. A7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   A8 .. AF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   B0 .. B7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   B8 .. BF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   C0 .. C7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   C8 .. CF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   D0 .. D7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   D8 .. DF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E0 .. E7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   E8 .. EF   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,   /*   F0 .. F7   */
   MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  MB12,  000,    /*   F8 .. FF   */
/* Else 0 cleared   */
#endif
#endif
#endif

};

#else   /* SYSTEM != SYS_MSDOS  */
/* For 32-bit or bigger systems.    */

const short   * type;       /* Pointer to one of the following type_*[].    */

#define EJ1     0x100   /* 1st byte of EUC_JP   */
#define EJ2     0x200   /* 2nd byte of EUC_JP   */
#define GB1     0x400   /* 1st byte of GB2312   */
#define GB2     0x800   /* 2nd byte of GB2312   */
#define KS1     0x1000  /* 1st byte of KSC5601  */
#define KS2     0x2000  /* 2nd byte of KSC5601  */

#define EJ12    (EJ1 | EJ2)     /* 1st byte or 2nd byte of EUC_JP   */
#define GB12    (GB1 | GB2)
#define KS12    (KS1 | KS2)
#define EU12    (EJ12 | GB12 | KS12)
    /* 1st or 2nd byte of EUC_JP, GB2312 or KSC5601 */

const short     type_euc[ UCHARMAX + 1] = {
/*
 * For EUC_JP, GB2312, KSC5601 or other similar multi-byte char encodings.
 */

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   SPA,   SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
   000,   000,   000,   000,   LET,   000,   000,   SPA,    /* 18           */
   SPA,   PUNC,  QUO,   PUNC,  DOL,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 40 @ABCDEFG  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 48 HIJKLMNO  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 50 PQRSTUVW  */
   LET,   LET,   LET,   PUNC,  000,   PUNC,  PUNC,  LET,    /* 58 XYZ[\]^_  */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 60 `abcdefg  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 68 hijklmno  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 70 pqrstuvw  */
   LET,   LET,   LET,   PUNC,  PUNC,  PUNC,  PUNC,  000,    /* 78 xyz{|}~   */

   000,   000,   000,   000,   000,   000,   000,   000,    /*   80 .. 87   */
   000,   000,   000,   000,   000,   000,   EJ1,   000,    /*   88 .. 8F   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   90 .. 97   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   98 .. 9F   */
   000,   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   A0 .. A7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   A8 .. AF   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   B0 .. B7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   B8 .. BF   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   C0 .. C7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   C8 .. CF   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   D0 .. D7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   D8 .. DF   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   E0 .. E7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   E8 .. EF   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,   /*   F0 .. F7   */
   EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  EU12,  000,    /*   F8 .. FF   */
};

const short     type_bsl[ UCHARMAX + 1] = {
/*
 * For SJIS, BIGFIVE or other similar encodings which may have '\\' value as
 * the second byte of multi-byte character.
 */

#define SJ1     0x100   /* 1st byte of SJIS     */
#define SJ2     0x200   /* 2nd byte of SJIS     */
#define BF1     0x400   /* 1st byte of BIGFIVE  */
#define BF2     0x800   /* 2nd byte of BIGFIVE  */

#define SB2     (SJ2 | BF2)
#define SJ12    (SJ1 | SJ2)
#define BF12    (BF1 | BF2)
#define SB12    (SJ12 | BF12)
#define S2B12   (SJ2 | BF1 | BF2)

#define LSB2    (LET | SB2)
#define PSB2    (PUNC| SB2)

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   SPA,   SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
   000,   000,   000,   000,   LET,   000,   000,   SPA,    /* 18           */
   SPA,   PUNC,  QUO,   PUNC,  DOL,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   SB2,   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 40 @ABCDEFG  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 48 HIJKLMNO  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 50 PQRSTUVW  */
   LSB2,  LSB2,  LSB2,  PSB2,  SB2,   PSB2,  PSB2,  LSB2,   /* 58 XYZ[\]^_  */
   SB2,   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 60 `abcdefg  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 68 hijklmno  */
   LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,  LSB2,   /* 70 pqrstuvw  */
   LSB2,  LSB2,  LSB2,  PSB2,  PSB2,  PSB2,  PSB2,  000,    /* 78 xyz{|}~   */

   SB2,   SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,   /*   80 .. 87   */
   SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,   /*   88 .. 8F   */
   SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,   /*   90 .. 97   */
   SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,  SJ12,   /*   98 .. 9F   */
   SJ2,   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   A0 .. A7   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   A8 .. AF   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   B0 .. B7   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   B8 .. BF   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   C0 .. C7   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   C8 .. CF   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   D0 .. D7   */
   S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12, S2B12,  /*   D8 .. DF   */
   SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,   /*   E0 .. E7   */
   SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,   /*   E8 .. EF   */
   SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,  SB12,   /*   F0 .. F7   */
   SB12,  SB12,  SB12,  SB12,  SB12,  BF12,  BF12,  000,    /*   F8 .. FF   */
};

/*
 * For ISO2022_JP multi-byte character encoding.
 */

#define IS1     0x40    /* 1st byte of shift-sequence   */
#define IS2     0x80    /* 2nd byte of shift-sequence   */
#define IS3     0x100   /* 3rd byte of shift-sequence   */
#define IS4     0x200   /* 4th byte of shift-sequence   */
#define IJ1     0x400   /* 1st byte of ISO-2022-JP (ISO-2022-JP1)   */
#define IJ2     0x800   /* 2nd byte of ISO-2022-JP (ISO-2022-JP1)   */

#define IJ12    (IJ1 | IJ2)

#define PI12    (PUNC | IJ12)
#define QI12    (QUO | IJ12)
#define DT12    (DOT | IJ12)
#define DG12    (DIG | IJ12)
#define LI12    (LET | IJ12)

#define DL12S2  (DOL | IJ12 | IS2)
#define P12S23  (PI12 | IS2 | IS3)
#define L12S3   (LI12 | IS3)
#define L12S4   (LI12 | IS4)

const short     type_iso2022_jp[ UCHARMAX + 1] = {

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   SPA,   SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
   000,   000,   000,   IS1,   LET,   000,   000,   SPA,    /* 18           */
   SPA,   PI12,  QI12,  PI12,  DL12S2,PI12,  PI12,  QI12,   /* 20  !"#$%&'  */
   P12S23,PI12,  PI12,  PI12,  PI12,  PI12,  DT12,  PI12,   /* 28 ()*+,-./  */
   DG12,  DG12,  DG12,  DG12,  DG12,  DG12,  DG12,  DG12,   /* 30 01234567  */
   DG12,  DG12,  PI12,  PI12,  PI12,  PI12,  PI12,  PI12,   /* 38 89:;<=>?  */

   IJ12,  LI12,  L12S3, LI12,  L12S4, LI12,  LI12,  LI12,   /* 40 @ABCDEFG  */
   LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,   /* 48 HIJKLMNO  */
   LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,   /* 50 PQRSTUVW  */
   LI12,  LI12,  LI12,  PI12,  IJ12,  PI12,  PI12,  LI12,   /* 58 XYZ[\]^_  */
   IJ12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,   /* 60 `abcdefg  */
   LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,   /* 68 hijklmno  */
   LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,  LI12,   /* 70 pqrstuvw  */
   LI12,  LI12,  LI12,  PI12,  PI12,  PI12,  PI12,  000,    /* 78 xyz{|}~   */
    /* the rests are 0 cleared  */
};

/*
 * For UTF8 multi-byte character encoding.
 */

#define U2_1    0x100   /* 1st byte of 2-byte encoding of UTF8  */
#define U2_2    0x200   /* 2nd byte of 2-byte encoding of UTF8  */
#define U3_1    0x400   /* 1st byte of 3-byte encoding of UTF8  */
#define U3_2    0x800   /* 2nd byte of 3-byte encoding of UTF8  */
#define U3_3    0x1000  /* 3rd byte of 3-byte encoding of UTF8  */

#define UT23    (U2_2 | U3_3)
/* 2nd byte of 2-byte encoding or 3rd byte of 3-byte encoding   */
#define UT223   (U2_2 | U3_2 | U3_3)
/* 2nd byte of 2-byte encoding, or 2nd or 3rd byte of 3-byte encoding   */

const short     type_utf8[ UCHARMAX + 1] = {

/* Character type codes */
/*   0,     1,     2,     3,     4,     5,     6,     7,                    */
/*   8,     9,     A,     B,     C,     D,     E,     F,       Hex          */

   000,   000,   000,   000,   000,   000,   000,   000,    /* 00           */
   000,   SPA,   SPA,   SPA,   SPA,   SPA,   000,   000,    /* 08           */
   000,   000,   000,   000,   000,   000,   000,   000,    /* 10           */
   000,   000,   000,   000,   LET,   000,   000,   SPA,    /* 18           */
   SPA,   PUNC,  QUO,   PUNC,  DOL,   PUNC,  PUNC,  QUO,    /* 20  !"#$%&'  */
   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  DOT,   PUNC,   /* 28 ()*+,-./  */
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,    /* 30 01234567  */
   DIG,   DIG,   PUNC,  PUNC,  PUNC,  PUNC,  PUNC,  PUNC,   /* 38 89:;<=>?  */

   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 40 @ABCDEFG  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 48 HIJKLMNO  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 50 PQRSTUVW  */
   LET,   LET,   LET,   PUNC,  000,   PUNC,  PUNC,  LET,    /* 58 XYZ[\]^_  */
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 60 `abcdefg  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 68 hijklmno  */
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET,    /* 70 pqrstuvw  */
   LET,   LET,   LET,   PUNC,  PUNC,  PUNC,  PUNC,  000,    /* 78 xyz{|}~   */

   UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,   /*   80 .. 87   */
   UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,   /*   88 .. 8F   */
   UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,   /*   90 .. 97   */
   UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,  UT23,   /*   98 .. 9F   */
   UT223, UT223, UT223, UT223, UT223, UT223, UT223, UT223,  /*   A0 .. A7   */
   UT223, UT223, UT223, UT223, UT223, UT223, UT223, UT223,  /*   A8 .. AF   */
   UT223, UT223, UT223, UT223, UT223, UT223, UT223, UT223,  /*   B0 .. B7   */
   UT223, UT223, UT223, UT223, UT223, UT223, UT223, UT223,  /*   B8 .. BF   */
   000,   000,   U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,   /*   C0 .. C7   */
   U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,   /*   C8 .. CF   */
   U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,   /*   D0 .. D7   */
   U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,  U2_1,   /*   D8 .. DF   */
   U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,   /*   E0 .. E7   */
   U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,  U3_1,   /*   E8 .. EF   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   F0 .. F7   */
   000,   000,   000,   000,   000,   000,   000,   000,    /*   F8 .. FF   */
};

#define SETLOCALE       2       /* #pragma setlocale (not __setlocale)  */

#define NUM_ENCODING    8
#define NUM_ALIAS       9

/*
 * Names of encoding recognized.  Table for search_encoding().
 * Note: GNU C documents that LANG=C-EUCJP (C-SJIS, C-JIS) is effective,
 * though this feature is not fully enabled in GNU C.
 */
static char *   encoding_name[ NUM_ENCODING][ NUM_ALIAS] = {
    /* normalized LANG, Visual C full, Visual C short
        , miscellaneous  */
    { "c",        "english",    "c"
        , "c",      "en",   "latin",    "iso8859",  "",     "" },
    { "ceucjp",  "",     ""
        , "eucjp",  "euc",  "ujis",     "jajpeucjp",    "",     "" },
    { "",         "chinesesimplified",  "chs"
        , "gb2312", "cngb",     "euccn",    "zhcn",     "zhcneuccn",    "" },
    { "",         "korean",   "kor"
        , "ksc5601",    "ksx1001",  "wansung",  "euckr",    "kokr", "kokreuckr" },
    { "csjis",    "japanese", "jpn"
        , "sjis",   "shiftjis", "mskanji",  "jajpsjis", "",     "" },
    { "",         "chinesetraditional", "cht"
        , "bigfive",    "big5", "cnbig5",   "euctw",    "zhtw", "zhtweuctw" },
    { "cjis",     "",     ""
        , "iso2022jp",  "iso2022jp1",   "jis",  "jajpjis",  "",     "" },
    { "",         "",     ""
        , "utf8",   "utf",      "",     "",     "",     "" },
};

#endif  /* SYSTEM != SYS_MSDOS  */

static int      mb2;

#if PROTO

static size_t   mb_read_2byte( int c1, char ** in_pp, char ** out_pp);
#if SYSTEM != SYS_MSDOS
static char *   search_encoding( char * norm, int alias);
static void     strip_bar( char * norm);
static size_t   mb_read_iso2022_jp( int c1, char ** in_pp, char ** out_pp);
static size_t   mb_read_utf8( int c1, char ** in_pp, char ** out_pp);
#endif

#else

static size_t   mb_read_2byte();    /* For 2-byte encodings of mbchar   */
#if SYSTEM != SYS_MSDOS
static char *   search_encoding();  /* Search encoding_name[][] table   */
static void     strip_bar();    /* Remove '_', '-' or '.' in the string */
static size_t   mb_read_iso2022_jp();   /* For ISO2022_JP encoding      */
static size_t   mb_read_utf8();         /* For UTF8 mbchar encoding     */
#endif

#endif

#if SYSTEM != SYS_MSDOS

#define NAMLEN  20

char *
#if PROTO
set_encoding( char * name, char * env, int pragma)
#else
set_encoding( name, env, pragma)
    char *  name;       /* Name of encoding specified   */
    char *  env;        /* Name of environment variable */
    int     pragma;
        /* 2: #pragma setlocale, 1: #pragma __setlocale, 0: not #pragma */
#endif
/*
 * Search the encoding specified and re-initialize mbchar settings.
 */
{
    const char *    unknown_encoding
            = "Unknown encoding: %s%.0ld%.0s";          /* _W1_ */
    const char *    too_long
            = "Too long encoding name: %s%.0ld%.0s";    /* _E_  */
    int     alias;
    char *  loc = "";
    char    norm[ NAMLEN];
                /* Normalized name (stripped '_', '-', '.' and lowered  */

    if (strlen( name) >= NAMLEN) {
        if ((env || pragma) && (warn_level & 1)) {
            cwarn( too_long, name, 0L, NULLST);
        } else {
            fprintf( fp_err, too_long, name);
            fputc( '\n', fp_err);
        }
    }
    strcpy( norm, name);
    conv_case( norm, norm + strlen( norm), LOWER);
    strip_bar( norm);

    if (strlen( name) == 0) {                       /* ""       */
        mbchar = MBCHAR;    /* Restore to the default encoding  */
    } else if (memcmp( norm, "iso8859", 7) == 0     /* iso8895* */
            || memcmp( norm, "latin", 5) == 0       /* latin*   */
            || memcmp( norm, "en", 2) == 0) {       /* en*      */
        mbchar = 0;                 /* No multi-byte character  */
    } else if (str_eq( norm + strlen( norm) - 4, "utf8")) {
        mbchar = UTF8;                              /* *utf8    */
    } else {
        alias = 3;
#if COMPILER == GNUC
        if (env && str_eq( env, "LANG"))
            alias = 0;
#endif
#if COMPILER == MSC
        if (pragma == SETLOCALE)        /* #pragma setlocale    */
            alias = 1;
#endif
        loc = search_encoding( norm, alias);        /* Search the name  */
    }
    if (loc == NULL) {
        if ((env || pragma) && (warn_level & 1)) {
            cwarn( unknown_encoding, name, 0L, NULLST);
        } else {                        /* -m option            */
            fprintf( fp_err, unknown_encoding, name);
            fputc( '\n', fp_err);
        }
    } else {
        mb_init();                      /* Re-initialize        */
    }
    return  loc;
}

static char *
#if PROTO
search_encoding( char * norm, int alias)
#else
search_encoding( norm, alias)
    char *  norm;           /* The name of encoding specified   */
    int     alias;          /* The number of alias to start searching   */
#endif
{
    char *  loc;
    int     lo, al;

    for (lo = 0; lo < NUM_ENCODING; lo++) {
        for (al = alias ; al < NUM_ALIAS; al++) {
            loc = encoding_name[ lo][ al];
            if (str_eq( loc, norm)) {
                switch (lo) {
                case 0  :   mbchar = 0;             break;
                case 1  :   mbchar = EUC_JP;        break;
                case 2  :   mbchar = GB2312;        break;
                case 3  :   mbchar = KSC5601;       break;
                case 4  :   mbchar = SJIS;          break;
                case 5  :   mbchar = BIGFIVE;       break;
                case 6  :   mbchar = ISO2022_JP;    break;
                case 7  :   mbchar = UTF8;          break;
                }
                return  loc;
            }
#if COMPILER == GNUC
            if (alias == 0 && al == 0)  /* Searched the names for LANG  */
                al = 2;         /* Skip the name for Visual C   */
#endif
        }
    }
    return  NULL;
}

static void
#if PROTO
strip_bar( char * string)
#else
strip_bar( string)
    char *  string;
#endif
/*
 * Strip '_', '-' or '.' in the string.
 */
{
    char *  cp = string;

    while (*cp != EOS) {
        if (*cp == '_' || *cp == '-' || *cp == '.')
            memmove( cp, cp + 1, strlen( cp));
        else
            cp++;
    }
}

#endif  /* SYSTEM != SYS_MSDOS  */

void
#if PROTO
mb_init( void)
#else
mb_init()
#endif
/*
 * Initialize multi-byte character settings.
 */
{
#if SYSTEM == SYS_MSDOS
    mb_read = mb_read_2byte;
#if MBCHAR == SJIS || MBCHAR == BIGFIVE
    bsl_in_mbchar = TRUE;   /* Multi-byte character may contain '\\' byte   */
#endif
#else
    /*
     * Select the character classification table, select the multi-byte
     * character reading routine and decide whether multi-byte character
     * may contain the byte of value 0x5c.
     */
    switch (mbchar) {
    case (0)    :
    case (EUC_JP)   :
    case (GB2312)   :
    case (KSC5601)  :
        type = type_euc;
        bsl_in_mbchar = FALSE;
        mb_read = mb_read_2byte;
        break;
    case (SJIS) :
    case (BIGFIVE)  :
        type = type_bsl;
        bsl_in_mbchar = TRUE;
        mb_read = mb_read_2byte;
        break;
    case (ISO2022_JP)    :
        type = type_iso2022_jp;
        bsl_in_mbchar = TRUE;
        mb_read = mb_read_iso2022_jp;
        break;
    case (UTF8) :
        type = type_utf8;
        bsl_in_mbchar = FALSE;
        mb_read = mb_read_utf8;
        break;
    }
#endif

    /* Set the bit patterns for character classification.   */
    switch (mbchar) {
    case (0)    :
        mbstart = 0;
        mbmask = ~0;
        break;
#if SYSTEM == SYS_MSDOS
    default :
        mbstart = MB1;
        mbmask = ~MB12;
        mb2 = MB2;
        break;
#else
    case (EUC_JP)   :
        mbstart = EJ1;
        mbmask = ~EU12;
        mb2 = EJ2;
        break;
    case (GB2312)   :
        mbstart = GB1;
        mbmask = ~EU12;
        mb2 = GB2;
        break;
    case (KSC5601)  :
        mbstart = KS1;
        mbmask = ~EU12;
        mb2 = KS2;
        break;
    case (SJIS) :
        mbstart = SJ1;
        mbmask = ~SB12;
        mb2 = SJ2;
        break;
    case (BIGFIVE)  :
        mbstart = BF1;
        mbmask = ~SB12;
        mb2 = BF2;
        break;
    case (ISO2022_JP)    :
        mbstart = IS1;
        mbmask = ~(IS1 | IS2 | IS3 | IS4 | IJ12);
        break;
    case (UTF8) :
        mbstart = (U2_1 | U3_1);
        mbmask = ~(U2_1 | U2_2 | U3_1 | U3_2 | U3_3);
        break;
#endif
    }

    /*
     * Set special handling for some encodings to supplement some compiler's
     * deficiency.
     */
    switch (mbchar) {
    case (SJIS) :
#if ! SJIS_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
    case (BIGFIVE)  :
#if ! BIGFIVE_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
#if SYSTEM != SYS_MSDOS
    case (ISO2022_JP)    :
#if ! ISO2022_JP_IS_ESCAPE_FREE
        bsl_need_escape = TRUE;
#endif
        break;
#endif
    default :
        bsl_need_escape = FALSE;
        break;
    }
}

static size_t
#if PROTO
mb_read_2byte( int c1, char ** in_pp, char ** out_pp)
#else
mb_read_2byte( c1, in_pp, out_pp)
    int     c1;         /* The 1st byte of mbchar sequence (already read)   */
    char ** in_pp;              /* Pointer to input     */
    char ** out_pp;             /* Pointer to output    */
#endif
/*
 * Multi-byte character reading routine for 2-byte encodings.
 */
{
    int     error = FALSE;
    size_t  len = 0;    /* Number of multi-byte characters read.    */
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;

    do {
        if (! (type[ (*out_p++ = *in_p++) & UCHARMAX] & mb2)) {
            error = TRUE;
            break;
        }
        len++;
    } while (type[ (*out_p++ = *in_p++) & UCHARMAX] & mbstart);
    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

#if SYSTEM != SYS_MSDOS

static size_t
#if PROTO
mb_read_iso2022_jp( int c1, char ** in_pp, char ** out_pp)
#else
mb_read_iso2022_jp( c1, in_pp, out_pp)
    int     c1; /* The 1st byte of the sequence already read (always 0x1b). */
    char ** in_pp;
    char ** out_pp;
#endif
/*
 * Multi-byte character reading routine for ISO2022_JP.
 */
{
    int     error = FALSE;
    size_t  len = 0;
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;
    int     c2, c3, c4;

    do {

        *out_p++ = c2 = *in_p++;
        if (! (type[ c2 & UCHARMAX] & IS2)) {
            error = TRUE;
            break;
        }
        *out_p++ = c3 = *in_p++;
        if (! (type[ c3 & UCHARMAX] & IS3)) {
            error = TRUE;
            break;
        }

        switch (c2) {
        case (0x24) :
            switch (c3) {
            case (0x42) :   /* 0x1b 0x24 0x42:  JIS X 0208-1983 */
                break;
            case (0x28) :
                *out_p++ = c4 = *in_p++;
                if (! (type[ c4 & UCHARMAX] & IS4))
                    error = TRUE;
                /* else:    0x1b 0x24 0x28 0x44:    JIS X 0212  */
                break;
            default :
                error = TRUE;
            }
            break;
        case (0x28) :
            switch (c3) {
            case (0x42) :   /* 0x1b 0x28 0x42:  ASCII   */
                c1 = *out_p++ = *in_p++ & UCHARMAX;
                continue;
            default :
                error = TRUE;
            }
            break;
        }
        if (error)
            break;

        while (type[ c1 = *out_p++ = (*in_p++ & UCHARMAX)] & IJ1) {
            if (! (type[ *out_p++ = (*in_p++ & UCHARMAX)] & IJ2)) {
                error = TRUE;
                break;
            }
            len++;          /* String of multi-byte characters  */
        }
        if (error)
            break;

    } while (type[ c1] & IS1);      /* 0x1b:    start of shift-sequence */

    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

static size_t
#if PROTO
mb_read_utf8( int c1, char ** in_pp, char ** out_pp)
#else
mb_read_utf8( c1, in_pp, out_pp)
    int     c1;
    char ** in_pp;
    char *  out_pp;
#endif
/*
 * Multi-byte character reading routine for UTF8.
 */
{
    int     error = FALSE;
    size_t  len = 0;
    char *  in_p = *in_pp;
    char *  out_p = *out_pp;
    int     c2;

    do {
        *out_p++ = c2 = *in_p++;
        if (type[ c1 & UCHARMAX] & U2_1) {      /* 2-byte character */
            if (! (type[ c2 & UCHARMAX] & U2_2)) {
                error = TRUE;
                break;
            }
        } else {
            if (type[ c2 & UCHARMAX] & U3_2) {  /* 3-byte character */
                if (! (type[ (*out_p++ = *in_p++) & UCHARMAX] & U3_3)) {
                    error = TRUE;
                    break;
                }
            } else {
                error = TRUE;
                break;
            }
        }
        len++;
    } while (type[ (*out_p++ = c1 = *in_p++) & UCHARMAX] & mbstart);
                        /* Start of the next multi-byte character   */

    *in_pp = --in_p;
    *(--out_p) = EOS;
    *out_pp = out_p;
    return  error ? (len | MB_ERROR) : len;
}

#endif  /* SYSTEM != SYS_MSDOS */

#if MODE != POST_STANDARD

uexpr_t
#if PROTO
mb_eval( char ** seq_pp)
#else
mb_eval( seq_pp)
    char ** seq_pp;
#endif
/*
 * Evaluate the value of a multi-byte character.
 * This routine does not check the legality of the sequence.
 * This routine is called from eval_char().
 */
{
    char *      seq = *seq_pp;
    uexpr_t     val = 0;
    int         c, c1;

    switch (mbchar) {
    case (EUC_JP)   :
    case (GB2312)   :
    case (KSC5601)  :
    case (SJIS) :
    case (BIGFIVE)  :
        val = (*seq++ & UCHARMAX) << 8;
        val += *seq++ & UCHARMAX;       /* Evaluate the 2-byte sequence */
        break;
#if SYSTEM != SYS_MSDOS
    case (ISO2022_JP)   :
        if (type[ c = *seq++ & UCHARMAX] & IS1) {   /* Skip shift-sequence  */
            if (type[ c = *seq++ & UCHARMAX] & IS2) {
                if (type[ c1 = *seq++ & UCHARMAX] & IS3) {
                    if (c1 == 0x28)
                        seq++;
                    if (c == 0x28 && c1 == 0x42) {  /* Shift-out sequence   */
                        val = 0;
                        break;
                    }
                    c = *seq++ & UCHARMAX;
                }
            }
        }
        val = (c << 8) + (*seq++ & UCHARMAX);       /* Evaluate the 2-byte  */
        break;
    case (UTF8) :       /* Evaluate the sequence of 2 or 3 bytes as it is   */
        if (type[ c = *seq++ & UCHARMAX] & U2_1) {
            val = (c << 8) + (*seq++ & UCHARMAX);
        } else {
            val = (c << 8) + (*seq++ & UCHARMAX);
            val = (val << 8) + (*seq++ & UCHARMAX);
        }
        break;
#endif
    }

    *seq_pp = seq;
    return  val;
}

#endif  /* MODE != POST_STANDARD    */
