#if _MIPS_SZPTR == 32
#define P32(addr) (addr)
#define LA la
#define LI_U32 li
#define MATC0 mtc0
#define MAFC0 mfc0
#define ADDAIU addiu
#define ADDAU addu
#elif _MIPS_SZPTR == 64
#define P32(addr) (0xFFFFffff00000000 | (addr))
#define LA dla
#define LI_U32 dli
#define MATC0 dmtc0
#define MAFC0 dmfc0
#define ADDAIU daddiu
#define ADDAU daddu
#else
#error Unknown MIPS address size
#endif
