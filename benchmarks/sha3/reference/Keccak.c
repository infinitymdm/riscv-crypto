/*
Implementation by the Keccak Team, namely, Guido Bertoni, Joan Daemen,
Michaël Peeters, Gilles Van Assche and Ronny Van Keer,
hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <stdio.h>
#include <byteswap.h>

#include "sha3/Keccak.h"

/*!
@addtogroup crypto_hash_sha3_reference SHA3 Reference
@brief Reference implementation of SHA3.
@ingroup crypto_hash_sha3
@{
*/

/*
================================================================
A readable and compact implementation of the Keccak-f[1600] permutation.
================================================================
*/


#define ROL64(a, offset) ((((uint64_t)a) << offset) ^ (((uint64_t)a) >> (64-offset)))
#define index(x, y) (((x)%5)+5*(((y)%5)))

#define readLane(x, y)          (((uint64_t*)state)[index(x, y)])
#define writeLane(x, y, lane)   (((uint64_t*)state)[index(x, y)]) = (lane)
#define XORLane(x, y, lane)     (((uint64_t*)state)[index(x, y)]) ^= (lane)

static const uint64_t KeccakP1600RoundConstants[24] =
{
    0x0000000000000001,
    0x0000000000008082,
    0x800000000000808a,
    0x8000000080008000,
    0x000000000000808b,
    0x0000000080000001,
    0x8000000080008081,
    0x8000000000008009,
    0x000000000000008a,
    0x0000000000000088,
    0x0000000080008009,
    0x000000008000000a,
    0x000000008000808b,
    0x800000000000008b,
    0x8000000000008089,
    0x8000000000008003,
    0x8000000000008002,
    0x8000000000000080,
    0x000000000000800a,
    0x800000008000000a,
    0x8000000080008081,
    0x8000000000008080,
    0x0000000080000001,
    0x8000000080008008,
};

static const unsigned int KeccakP1600RhoOffsets[25] =
{
  0,  // 0   *
  1,  // 1
 62,  // 2
 28,  // 3
 27,  // 4
 36,  // 5   *
 44,  // 6
  6,  // 7
 55,  // 8
 20,  // 9
  3,  // 10  *
 10,  // 11
 43,  // 12
 25,  // 13
 39,  // 14
 41,  // 15  *
 45,  // 16
 15,  // 17
 21,  // 18
  8,  // 19
 18,  // 20  *
  2,  // 21
 61,  // 22
 56,  // 23
 14   // 24
};


/**
 * Function that computes the Keccak-f[1600] permutation on the given state.
 */
static void KeccakF1600_StatePermute(void *state)
{
    int round, x, y;

    for(round=0; round<24; round++) {
        uint64_t C[5];
        uint64_t tempA[25];
        uint64_t D;

        printf("x:\n");
        for (int i = 0; i < 200; i++) printf("%02x", ((uint8_t *)state)[i]);
        printf("\n");

        // Theta / Rho / Pi

        for(x=0; x<5; x++) {
            C[x] = ((uint64_t*)state)[index(x, 0)] ^
                   ((uint64_t*)state)[index(x, 1)] ^
                   ((uint64_t*)state)[index(x, 2)] ^
                   ((uint64_t*)state)[index(x, 3)] ^
                   ((uint64_t*)state)[index(x, 4)] ;
        }

        printf("Theta / Rho:\n");
        for(x=0; x<5; x++) {

            D = ROL64(C[(x+1)%5], 1) ^ C[(x+4)%5];

            for(y=0; y<5; y++) {
                printf("T: %064llb\n", __bswap_64(((uint64_t*)state)[index(x, y)] ^ D));
                printf("R: %064llb (%u)\n\n", __bswap_64(ROL64(((uint64_t*)state)[index(x, y)] ^ D, KeccakP1600RhoOffsets[index(x, y)])), KeccakP1600RhoOffsets[index(x, y)]);

                tempA[index(0*x+1*y, 2*x+3*y)] =
                    ROL64 (
                        ((uint64_t*)state)[index(x, y)] ^ D,
                        KeccakP1600RhoOffsets[index(x, y)]
                    );
            }
        }
        printf("\n");

        for (int i = 0; i < 25; i++) printf("%x", __bswap_64(tempA[i]));
        

        // Chi

        for(y=0; y<5; y++) {
            for(x=0; x<5; x++) {
                ((uint64_t*)state)[index(x, y)] =
                    tempA[index(x, y)] ^
                        ((~tempA[index(x+1, y)]) &
                           tempA[index(x+2, y)]);
            }
        }

        // printf("Chi:\n");
        // for (int i = 0; i < 200; i++) printf("%02x", ((uint8_t *)state)[i]);
        // printf("\n");

        // Iota
        ((uint64_t*)state)[index(0, 0)] ^= KeccakP1600RoundConstants[round];

        // printf("Iota:\n");
        // for (int i = 0; i < 200; i++) printf("%02x", ((uint8_t *)state)[i]);
        printf("\n\n");
    }
}

/*
================================================================
A readable and compact implementation of the Keccak sponge functions
that use the Keccak-f[1600] permutation.
================================================================
*/

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void Keccak(unsigned int rate, unsigned int capacity, const unsigned char *input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char *output, unsigned long long int outputByteLen)
{
    uint8_t state[200];
    unsigned int rateInBytes = rate/8;
    unsigned int blockSize = 0;
    unsigned int i;

    if (((rate + capacity) != 1600) || ((rate % 8) != 0))
        return;

    /* === Initialize the state === */
    memset(state, 0, sizeof(state));

    /* === Absorb all the input blocks === */
    while(inputByteLen > 0) {
        blockSize = MIN(inputByteLen, rateInBytes);
        printf("Message chunk: ");
        for(i=0; i<blockSize; i++) {
            state[i] ^= input[i];
            printf("%02x", input[i]);
        }
        printf("\n");
        input += blockSize;
        inputByteLen -= blockSize;

        if (blockSize == rateInBytes) {
            KeccakF1600_StatePermute(state);
            blockSize = 0;
        }
    }

    /* === Do the padding and switch to the squeezing phase === */
    /* Absorb the last few bits and add the first bit of padding (which coincides with the delimiter in delimitedSuffix) */
    state[blockSize] ^= delimitedSuffix;
    /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */
    if (((delimitedSuffix & 0x80) != 0) && (blockSize == (rateInBytes-1)))
        KeccakF1600_StatePermute(state);
    /* Add the second bit of padding */
    state[rateInBytes-1] ^= 0x80;
    /* Switch to the squeezing phase */
    KeccakF1600_StatePermute(state);

    /* === Squeeze out all the output blocks === */
    while(outputByteLen > 0) {
        blockSize = MIN(outputByteLen, rateInBytes);
        memcpy(output, state, blockSize);
        output += blockSize;
        outputByteLen -= blockSize;

        if (outputByteLen > 0)
            KeccakF1600_StatePermute(state);
    }
}

/*! @} */
