
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sha3/fips202.h"

int main(int argc, char **argv) {

    FILE               *message_file;
    unsigned long long  message_len;
    unsigned char      *message;
    unsigned char       digest[64];

    // Open file argv[1]
    message_file = fopen(argv[1], "rb");
    if (!message_file) {
        fprintf(stderr, "Unable to open file %s", argv[1]);
        return 1;
    }

    // Get file length
    fseek(message_file, 0, SEEK_END);
    message_len = ftell(message_file);
    fseek(message_file, 0, SEEK_SET);

    // Read file contents into message
    message = (unsigned char *) malloc(message_len+1);
    fread(message, message_len, 1, message_file);
    fclose(message_file);

    // Hash message
    FIPS202_SHA3_512(message, message_len, digest);

    // Display digest
    for (int i = 0; i < 64; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");

    free(message);
    return 0;
}
