#include "3264.h"

struct {
	uint8_t dmem[0x1000];
	uint8_t imem[0x1000];
	uint8_t _unused04002000[0x40000-0x2000];
	uint32_t spmemaddr;
	uint32_t dramaddr;
	uint32_t dram2spmem; /* 0xssscclll: skip, count, length */
	uint32_t spmem2dram; /* 0xssscclll: skip, count, length */
	uint32_t status;
	uint32_t dmafull;
	uint32_t dmabusy;
	uint32_t semaphore;
	uint8_t _unused04040020[0x80000-0x40020];
	uint32_t pc;
	uint32_t ibist; /* IMEM Built-In-Self-Test */
} static volatile * const SP = (void*)P32(0xA4000000);
enum SP_STATUS_READ {
	SP_STATUS_HALT         = 1 << 0,
	SP_STATUS_BROKE        = 1 << 1,
	SP_STATUS_DMA_BUSY     = 1 << 2,
	SP_STATUS_DMA_FULL     = 1 << 3,
	SP_STATUS_IO_FULL      = 1 << 4,
	SP_STATUS_SINGLE_STEP  = 1 << 5,
	SP_STATUS_INT_ON_BREAK = 1 << 6,
	SP_STATUS_SIGNAL0      = 1 << 7,
	SP_STATUS_SIGNAL1      = 1 << 8,
	SP_STATUS_SIGNAL2      = 1 << 9,
	SP_STATUS_SIGNAL3      = 1 << 10,
	SP_STATUS_SIGNAL4      = 1 << 11,
	SP_STATUS_SIGNAL5      = 1 << 12,
	SP_STATUS_SIGNAL6      = 1 << 13,
	SP_STATUS_SIGNAL7      = 1 << 14,
};
enum SP_STATUS_WRITE {
	SP_STATUS_CLEAR_HALT         = 1 << 0,
	SP_STATUS_SET_HALT           = 1 << 1,
	SP_STATUS_CLEAR_BROKE        = 1 << 2,
	SP_STATUS_CLEAR_INT          = 1 << 3,
	SP_STATUS_SET_INT            = 1 << 4,
	SP_STATUS_CLEAR_SINGLE_STEP  = 1 << 5,
	SP_STATUS_SET_SINGLE_STEP    = 1 << 6,
	SP_STATUS_CLEAR_INT_ON_BREAK = 1 << 7,
	SP_STATUS_SET_INT_ON_BREAK   = 1 << 8,
	SP_STATUS_CLEAR_SIGNAL0      = 1 << 9,
	SP_STATUS_SET_SIGNAL0        = 1 << 10,
	SP_STATUS_CLEAR_SIGNAL1      = 1 << 11,
	SP_STATUS_SET_SIGNAL1        = 1 << 12,
	SP_STATUS_CLEAR_SIGNAL2      = 1 << 13,
	SP_STATUS_SET_SIGNAL2        = 1 << 14,
	SP_STATUS_CLEAR_SIGNAL3      = 1 << 15,
	SP_STATUS_SET_SIGNAL3        = 1 << 16,
	SP_STATUS_CLEAR_SIGNAL4      = 1 << 17,
	SP_STATUS_SET_SIGNAL4        = 1 << 18,
	SP_STATUS_CLEAR_SIGNAL5      = 1 << 19,
	SP_STATUS_SET_SIGNAL5        = 1 << 20,
	SP_STATUS_CLEAR_SIGNAL6      = 1 << 21,
	SP_STATUS_SET_SIGNAL6        = 1 << 22,
	SP_STATUS_CLEAR_SIGNAL7      = 1 << 23,
	SP_STATUS_SET_SIGNAL7        = 1 << 24,
};

struct {
	uint32_t start;
	uint32_t end;
	uint32_t current;
	uint32_t status;
	uint32_t clock; /* clock counter */
	uint32_t bufbusy; /* buffer busy counter */
	uint32_t pipebusy; /* pipe busy counter */
	uint32_t tmem; /* tmem load counter */
} static volatile * const DP = (void*)P32(0xA4100000);
enum DP_STATUS_READ {
	DP_STATUS_XBUS_DMEM_DMA = 1 << 0,
	DP_STATUS_FREEZE        = 1 << 1,
	DP_STATUS_FLUSH         = 1 << 2,
	DP_STATUS_START_GCLK    = 1 << 3,
	DP_STATUS_TMEM_BUSY     = 1 << 4,
	DP_STATUS_PIPE_BUSY     = 1 << 5,
	DP_STATUS_CMD_BUSY      = 1 << 6,
	DP_STATUS_CBUF_READY    = 1 << 7,
	DP_STATUS_DMA_BUSY      = 1 << 8,
	DP_STATUS_END_VALID     = 1 << 9,
	DP_STATUS_START_VALID   = 1 << 10,
};
enum DP_STATUS_WRITE {
	DP_STATUS_CLEAR_XBUS_DMEM_DMA = 1 << 0, /* rdram->dp */
	DP_STATUS_SET_XBUS_DMEM_DMA   = 1 << 1, /* dmem->dp */
	DP_STATUS_CLEAR_FREEZE        = 1 << 2,
	DP_STATUS_SET_FREEZE          = 1 << 3,
	DP_STATUS_CLEAR_FLUSH         = 1 << 4,
	DP_STATUS_SET_FLUSH           = 1 << 5,
	DP_STATUS_CLEAR_TMEM_CTR      = 1 << 6,
	DP_STATUS_CLEAR_PIPE_CTR      = 1 << 7,
	DP_STATUS_CLEAR_CMD_CTR       = 1 << 8,
	DP_STATUS_CLEAR_CLOCK_CTR     = 1 << 9,
};

struct {
	uint32_t mode;
	uint32_t version;
	uint32_t intr;
	uint32_t intrmask;
} static volatile * const MI = (void*)P32(0xA4300000);
enum MI_INTR_READ {
	MI_INTR_SP = 1 << 0,
	MI_INTR_SI = 1 << 1,
	MI_INTR_AI = 1 << 2,
	MI_INTR_VI = 1 << 3,
	MI_INTR_PI = 1 << 4,
	MI_INTR_DP = 1 << 5,
};
enum MI_INTRMASK_WRITE {
	MI_INTRMASK_CLEAR_SP = 1 << 0,
	MI_INTRMASK_SET_SP = 1 << 1,
	MI_INTRMASK_CLEAR_SI = 1 << 2,
	MI_INTRMASK_SET_SI = 1 << 3,
	MI_INTRMASK_CLEAR_AI = 1 << 4,
	MI_INTRMASK_SET_AI = 1 << 5,
	MI_INTRMASK_CLEAR_VI = 1 << 6,
	MI_INTRMASK_SET_VI = 1 << 7,
	MI_INTRMASK_CLEAR_PI = 1 << 8,
	MI_INTRMASK_SET_PI = 1 << 9,
	MI_INTRMASK_CLEAR_DP = 1 << 10,
	MI_INTRMASK_SET_DP = 1 << 11,
};

struct {
	uint32_t control;
	uint32_t dramaddr;
	uint32_t width;
	uint32_t intr;
	uint32_t current;
	uint32_t timing;
	uint32_t vsync;
	uint32_t hsync;
	uint32_t hsync_leap;
	uint32_t hrange;
	uint32_t vrange;
	uint32_t vburstrange;
	uint32_t xscale;
	uint32_t yscale;
} static volatile * const VI = (void*)P32(0xA4400000);

struct {
	uint32_t dramaddr;
	uint32_t len;
	uint32_t control;
	uint32_t status;
	uint32_t dacrate;
	uint32_t bitrate;
} static volatile * const AI = (void*)P32(0xA4500000);
enum AI_CONTROL_WRITE {
	AI_CONTROL_DMA = 1 << 0,
};
enum AI_STATUS_READ {
	AI_STATUS_FULL = 1 << 31,
	AI_STATUS_BUSY = 1 << 30,
};

struct {
	uint32_t dramaddr;
	uint32_t cartaddr;
	uint32_t dram2cart; /* read (to PI) */
	uint32_t cart2dram; /* write (from PI) */
	uint32_t status;
	uint32_t dom1lat; /* latency */
	uint32_t dom1pwd; /* pulse width */
	uint32_t dom1pgs; /* page size */
	uint32_t dom1rls; /* release */
	uint32_t dom2lat; /* latency */
	uint32_t dom2pwd; /* pulse width */
	uint32_t dom2pgs; /* page size */
	uint32_t dom2rls; /* release */
} static volatile * const __attribute((unused)) PI = (void*)P32(0xA4600000);
enum PI_STATUS_READ {
	PI_STATUS_DMABUSY = 1 << 0,
	PI_STATUS_IOBUSY  = 1 << 1,
	PI_STATUS_ERROR   = 1 << 2,
};
enum PI_STATUS_WRITE {
	PI_STATUS_RESET     = 1 << 0,
	PI_STATUS_CLEARINTR = 1 << 1,
};

struct {
	uint32_t dramaddr;
	uint32_t pifram2dram;
	uint32_t _reserved08;
	uint32_t _reserved0c;
	uint32_t dram2pifram;
	uint32_t _reserved14;
	uint32_t status;
} static volatile * const SI = (void*)P32(0xA4800000);
enum SI_STATUS_READ {
	SI_STATUS_DMABUSY  = 1 <<  0,
	SI_STATUS_IOBUSY   = 1 <<  1,
	SI_STATUS_DMAERROR = 1 <<  3,
	SI_STATUS_INTR     = 1 << 12,
};

struct {
	uint32_t cfg;
	uint32_t status;
	uint32_t dmalen;
	uint32_t dmaaddr;
	uint32_t msg;
	uint32_t dmacfg;
	uint32_t spi;
	uint32_t spicfg;
	uint32_t key;
	uint32_t savcfg;
	uint32_t sec;
	uint32_t ver;
} static volatile * const ED64 = (void*)P32(0xA8040000);
enum ED64_CFG {
	ED64_CFG_SDRAM_ON     = 1 << 0,
	ED64_CFG_SWAP         = 1 << 1,
	ED64_CFG_WR_MOD       = 1 << 2,
	ED64_CFG_WR_ADDR_MASK = 1 << 3,
};
enum ED64_STATUS {
	ED64_STATUS_DMABUSY    = 1 << 0,
	ED64_STATUS_DMATIMEOUT = 1 << 1,
	ED64_STATUS_TXE        = 1 << 2,
	ED64_STATUS_RXF        = 1 << 3,
	ED64_STATUS_SPI        = 1 << 4,
};
enum ED64_DMACFG {
	ED64_DMACFG_SD_TO_RAM   = 1,
	ED64_DMACFG_RAM_TO_SD   = 2,
	ED64_DMACFG_FIFO_TO_RAM = 3,
	ED64_DMACFG_RAM_TO_FIFO = 4,
};
