	.set noat, reorder

	.text

# memmap:
#   0 \__consts
#  2F /
# ...
#  80 \__*achannels
#  83 /
#  84 \__*outbuf
#  87 /
# ...
#  C0 \__achannel (load on each achannel)
#  CF /
# ...
# 400 \__source audio (load on each achannel)
# 56F /
# ...
# 690 \
# 800 |__mixed audio
# 96F /
# ...

## AChannel_Offset_xxx
#define acopsample 0x00
#define acolen     0x04
#define acolooplen 0x06
#define acorate    0x08
#define acovol     0x0A
#define acoscale   0x0C
#define acofrac    0x0E
#define acsize     0x10

	.balign 8
	.globl rspaudio_text
	.type rspaudio_text, %function
rspaudio_text:

	vxor $v0, $v0, $v0
	lqv $v31[0], 0x000($zero) # consts+0

# clear outbuf
	la $11, 0x800 # 0x400
	li $12, 368 / 8

1:
	sqv $v0[0], 0($11)
	addi $11, 16
	addi $12, -1
	bnez $12, 1b

## merge achannels
	lqv $v20[0], 0x010($zero) # consts+0x10
	lqv $v21[0], 0x020($zero) # consts+0x20
	lw $20, 0x080($zero) # *achannels
	li $21, 32

achanloop:
	# load achannel
	li $4, 0x0C0
	mtc0 $4, $0
	mtc0 $20, $1
	li $4, acsize - 1
	mtc0 $4, $2
1:
	mfc0 $4, $6
	bnez $4, 1b

	# test channel is active
	lhu $13, 0x0C0+acolen($zero)
	beqz $13, quietchan

	# required nsamples = [len, 368].min
	li $14, 368
	sltu $5, $14, $13
	bnez $5, 1f
	move $14, $13
1:

	# load channel sample
	la $10, 0x400 # 0x400+(368/8)*16-(368/8)*4
	mtc0 $10, $0
	lw $15, 0x0C0+acopsample($zero)
	mtc0 $15, $1
	srl $4, $14, 1
	addi $4, -1
	mtc0 $4, $2
1:
	mfc0 $4, $6
	bnez $4, 1b

	# restore scale
	lh $4, 0x0C0+acoscale($zero)
	mtc2 $4, $v1[0]
	vor $v4, $v0, $v1[0]

	# decode and mix
	#la $10, 0x400 # 0x400+(368/8)*16-(368/8)*4
	la $11, 0x800 # 0x400
	srl $12, $14, 3 #li $12, 368 / 8
decloop:

# ABCD EFGH => sssA sssB sssC sssD sssE sssF sssG sssH
	llv $v1[0], 0($10)
	vmudn $v2, $v21, $v1[1]
	vmadn $v1, $v20, $v1[0]
	#vadd $v1, $v1, $v2 # merge => ABCD BCD0 CD00 D000 EFGH FGH0 GH00 H000 //vmad does this
	#vsaw $v1, $v1, $v1[0] # accL without clamp
	vmudm $v1, $v1, $v31[0] # s>> 12 => sssA sssB sssC sssD sssE sssF sssG sssH
# save...
	vadd $v2, $v1, $v0
# *= scale
	vmudn $v1, $v1, $v4
# mix-store
	lqv $v6[0], 0($11)
	vadd $v1, $v1, $v6
	sqv $v1[0], 0($11)
# calc scale-delta from saved
	# index = (v[0..7].abs - 3).clamp(0, 4).max
	vabs $v1, $v2, $v2
	vsub $v1, $v1, $v31[1] # -=3
	vge $v1, $v1, $v0 # clamp(0, *) using -=3 >= 0
	vge $v1, $v1, $v1[1q] # max 1/3: 01 11 23 33 45 55 67 77
	vge $v1, $v1, $v1[2h] # max 2/3: 0123 1123 2323 3323 4567 5567 6767 7767
	vge $v1, $v1, $v1[4] # max 3/3: 01234567 11234567 23234567 33234567 45674567 55674567 67674567 77674567
	# v2 = scaler[index]
	mfc2 $4, $v1[0]
	sll $4, 1
	lh $4, 0x0006($4) # const+6 = scaler
	mtc2 $4, $v1[0]
	vor $v2, $v0, $v1[0]
## v2:4.12 -> v3v1:16.16
	vmudn $v1, $v2, $v31[0]
	vmadh $v3, $v2, $v0
## scale *= v3v1
	vmudn $v30, $v1, $v4
	vmadh $v4, $v3, $v4

	addi $10, 4
	addi $11, 16
	addi $12, -1
	bnez $12, decloop

	## writeback achannel
	# regs->dmem
	addi $10, -0x400
	add $15, $10
	sw $15, 0x0C0+acopsample($zero)
	sub $13, $14
	# TODO loop etc
	slti $4, $13, 8
	beqz $4, 1f
	li $13, 0
1:
	sh $13, 0x0C0+acolen($zero)
	mfc2 $4, $v4[0]
	sh $4, 0x0C0+acoscale($zero)

	# dmem->rdram
	li $4, 0x0C0
	mtc0 $4, $0
	mtc0 $20, $1
	li $4, acsize - 1
	mtc0 $4, $3
1:
	mfc0 $4, $6
	bnez $4, 1b

quietchan:
	addi $20, acsize
	addi $21, -1
	bnez $21, achanloop

	# outbuf mono->stereo
	la $10, 0x800-368*2
	la $11, 0x800
	li $12, 368/4
1:
	lsv $v1[2*0], 0x00($11)
	lsv $v1[2*2], 0x02($11)
	lsv $v1[2*4], 0x04($11)
	lsv $v1[2*6], 0x06($11)
	vor $v1, $v0, $v1[0q]
	sqv $v1[0], 0($10)

	addi $10, 16
	addi $11, 8
	addi $12, -1
	bnez $12, 1b

	# put outbuf
	la $4, 0x800-368*2
	mtc0 $4, $0
	lw $4, 0x084($zero) # *outbuf
	mtc0 $4, $1
	li $4, 368 * 2 * 2 - 1 # 368[samples]*2[bytes/sample]*2[ch]
	mtc0 $4, $3
1:
	mfc0 $4, $6
	bnez $4, 1b

	break


	.globl rspaudio_text_size
	.set rspaudio_text_size, . - rspaudio_text


	.data

	.balign 8
	.globl rspaudio_data
	.type rspaudio_data, %object
rspaudio_data:

consts:
	.short 0x0010, 0x0003, 0x0000, 0x0E90, 0x1360, 0x1770, 0x1C50, 0x2250
	.short 0x0001, 0x0010, 0x0100, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000
	.short 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0010, 0x0100, 0x1000

	.globl rspaudio_data_size
	.set rspaudio_data_size, . - rspaudio_data
