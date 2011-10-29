
#define WTYPE  unsigned long

typedef struct
{
   int    maxlen;
   int    len;
   int    pos;
   WTYPE *data;
} BitList;

extern BitList *new      (int maxbits);
extern void      DESTROY  (BitList *pVector);

extern int       reserve_bits (BitList *d, int bits);
extern int       resize   (BitList *d, int bits);

extern int       getlen   (BitList *d);
extern int       getpos   (BitList *d);
extern int       getmaxlen(BitList *d);
extern int       setlen   (BitList *d, int newlen);
extern int       setpos   (BitList *d, int newpos);

extern WTYPE     vread    (BitList *d, int bits);
extern void      vwrite   (BitList *d, int bits, WTYPE value);

extern void      dump     (BitList *d);

extern WTYPE     get_unary (BitList *d);
extern void      put_unary (BitList *d, WTYPE value);
extern WTYPE     get_gamma (BitList *d);
extern void      put_gamma (BitList *d, WTYPE value);

extern void      put_string(BitList *d, char* s);
