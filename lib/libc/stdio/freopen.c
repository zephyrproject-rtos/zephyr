#include <stdio.h>

FILE *freopen(const char *filename, const char *mode, FILE *stream) {
    // If no stream is provided, simply open a new file.
    if (stream == NULL) {
        return fopen(filename, mode);
    }
    // Flush and close the current stream.
    fflush(stream);
    if (fclose(stream) != 0) {
        return NULL;  // Return NULL if closing fails.
    }
    // Open the new file with the specified mode.
    return fopen(filename, mode);
}
