
/*
 * Note: we have to be careful with our function names.  On HP/UX, for example,
 * if we have a function calles setpos, it will call the Fortran setpos
 * function instead of ours, leading to all sorts of fun errors.
 */

#define WTYPE  unsigned long

#define BITS_PER_WORD (8 * sizeof(WTYPE))
#define MAXBIT        (BITS_PER_WORD-1)
#define NWORDS(bits)  ( ((bits)+BITS_PER_WORD-1) / BITS_PER_WORD )
#define NBYTES(bits)  ( ((bits)+8-1) / 8 )

typedef enum
{
   eModeR,
   eModeRO,
   eModeW,
   eModeWO,
   eModeRW,
   eModeA,
} FileMode;

typedef struct
{
   int       maxlen;
   int       len;
   int       pos;
   WTYPE    *data;
   FileMode  mode;
   char*     file;
   char*     file_header;
   int       file_header_lines;
   int       is_writing;
} BitList;

extern BitList*  new            (int initial_maxlen);
extern void      DESTROY        (BitList *pVector);

extern int       resize         (BitList *l, int bits);

extern int       set_len        (BitList *l, int newlen);
extern int       set_pos        (BitList *l, int newpos);

extern void      read_open      (BitList *l);
extern void      write_open     (BitList *l);
extern void      write_close    (BitList *l);

extern WTYPE     sread          (BitList *l, int bits);
extern WTYPE     sreadahead     (BitList *l, int bits);
extern void      swrite         (BitList *l, int bits, WTYPE value);

extern void      dump           (BitList *l);

extern void      put_string     (BitList *l, char* s);
extern char*     read_string    (BitList *l, int bits);

extern char*     to_raw         (BitList *l);
extern void      from_raw       (BitList *l, char* str, int bits);

extern WTYPE     get_unary      (BitList *l);
extern WTYPE     get_unary1     (BitList *l);
extern WTYPE     get_gamma      (BitList *l);
extern WTYPE     get_delta      (BitList *l);
extern WTYPE     get_omega      (BitList *l);
extern WTYPE     get_fib        (BitList *l);
extern WTYPE     get_levenstein (BitList *l);
extern WTYPE     get_evenrodeh  (BitList *l);
extern WTYPE     get_binword    (BitList *l, int k);
extern WTYPE     get_baer       (BitList *l, int k);
extern WTYPE     get_rice       (BitList *l, int k);
extern WTYPE     get_golomb     (BitList *l, WTYPE m);
extern WTYPE     get_gamma_rice (BitList *l, int k);
extern WTYPE     get_gamma_golomb (BitList *l, WTYPE m);
extern WTYPE     get_adaptive_gamma_rice (BitList *l, int *k);

extern void      put_unary      (BitList *l, WTYPE value);
extern void      put_unary1     (BitList *l, WTYPE value);
extern void      put_gamma      (BitList *l, WTYPE value);
extern void      put_delta      (BitList *l, WTYPE value);
extern void      put_omega      (BitList *l, WTYPE value);
extern void      put_fib        (BitList *l, WTYPE value);
extern void      put_levenstein (BitList *l, WTYPE value);
extern void      put_evenrodeh  (BitList *l, WTYPE value);
extern void      put_binword    (BitList *l, int k, WTYPE value);
extern void      put_baer       (BitList *l, int k, WTYPE value);
extern void      put_rice       (BitList *l, int k, WTYPE value);
extern void      put_golomb     (BitList *l, WTYPE m, WTYPE value);
extern void      put_gamma_rice (BitList *l, int k, WTYPE value);
extern void      put_gamma_golomb (BitList *l, WTYPE m, WTYPE value);
extern void      put_adaptive_gamma_rice (BitList *l, int *k, WTYPE value);
