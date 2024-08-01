#ifndef GSACCESS
#define GSACCESS


#ifdef __cplusplus
extern "C" {
#endif

void  RealCPUID(Bit64u, void*);

Bit8u GSReadByte(Bit64u);
Bit16u GSReadWord(Bit64u);
Bit32u GSReadDword(Bit64u);
Bit64u GSReadQword(Bit64u);

void GSWriteByte(Bit64u, Bit8u);
void GSWriteWord(Bit64u, Bit16u);
void GSWriteDword(Bit64u, Bit32u);
void GSWriteQword(Bit64u, Bit64u);


#ifdef __cplusplus
}
#endif

extern Bit64u _PEB;
extern Bit64u _PEBend;
extern Bit64u _TEB;
extern Bit64u _TEBend;


#endif // GSACCESS

