
#define WTYPE  unsigned long

typedef struct
{
   int    maxlen;
   int    len;
   int    pos;
   WTYPE *data;
} BitList;

extern BitList *new        (int maxbits);
extern void      DESTROY   (BitList *pVector);

extern int       reserve_bits (BitList *d, int bits);
extern int       resize    (BitList *d, int bits);

extern int       getlen    (BitList *d);
extern int       getpos    (BitList *d);
extern int       getmaxlen (BitList *d);
extern int       setlen    (BitList *d, int newlen);
extern int       setpos    (BitList *d, int newpos);

extern WTYPE     vread     (BitList *d, int bits);
extern WTYPE     vreadahead(BitList *d, int bits);
extern void      vwrite    (BitList *d, int bits, WTYPE value);

extern void      dump      (BitList *d);

extern void      put_string(BitList *d, char* s);
extern char*     read_string(BitList *d, int bits);

extern char*     to_raw(BitList *d);
extern void      from_raw(BitList *d, char* s, int bits);

extern WTYPE     get_unary (BitList *d);
extern void      put_unary (BitList *d, WTYPE value);
extern WTYPE     get_unary1(BitList *d);
extern void      put_unary1(BitList *d, WTYPE value);
extern WTYPE     get_gamma (BitList *d);
extern void      put_gamma (BitList *d, WTYPE value);
extern WTYPE     get_delta (BitList *d);
extern void      put_delta (BitList *d, WTYPE value);
extern WTYPE     get_omega (BitList *d);
extern void      put_omega (BitList *d, WTYPE value);
extern WTYPE     get_fib (BitList *d);
extern void      put_fib (BitList *d, WTYPE value);
extern WTYPE     get_levenstein (BitList *d);
extern void      put_levenstein (BitList *d, WTYPE value);
extern WTYPE     get_adaptive_gamma_rice (BitList *d, int *k);
extern void      put_adaptive_gamma_rice (BitList *d, int *k, WTYPE value);
