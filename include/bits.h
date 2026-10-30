#ifndef PINS_H
#define PINS_H

/** Bit Set Masked: sets `bits` at offset `shift` in `target` after clearing them */
#define bsm(target, shift, mask, bits) ((target) = ((target) & ~((mask) << (shift))) | (((bits) & (mask)) << (shift)))

/** Converts two unsigned 8bit values into an unsigned 16bit value */
#define u8to16(msb, lsb) ((uint16_t)(msb) << 8 | (lsb))

/** Converts an unsigned 16bit value into two big endian 8bit values */
#define u16be(v) (uint8_t)((v) >> 8), (uint8_t)(v)

#endif
