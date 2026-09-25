#ifndef CODE_GEN_TESTS_UTILS_H
#define CODE_GEN_TESTS_UTILS_H
#include <cctype>
#include <cstdio>

#define ROW_LENGTH 32
#define DO_ASCII false
#define DO_ROW_OFFSETS false

inline void hexDump(const void *buffer, const size_t length) {
    const auto *data = static_cast<const unsigned char *>(buffer);
    size_t j;

    for (size_t i = 0; i < length; i += ROW_LENGTH) {
        // Print the offset
        if constexpr (DO_ROW_OFFSETS)
            printf("%08x ", static_cast<unsigned int>(i));

        // Print hex values
        for (j = 0; j < ROW_LENGTH; j++) {
            if (i + j < length)
                printf("%02x ", data[i + j]);
            else
                printf(" ");
        }

        // Print ASCII characters
        if constexpr (DO_ASCII) {
            printf(" |");
            for (j = 0; j < ROW_LENGTH; j++) {
                if (i + j < length)
                    printf("%c", isprint(data[i + j]) ? data[i + j] : '.');
                else
                    printf(" ");
            }
        }
        printf("\n");
    }
}

#endif