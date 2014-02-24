#include "pebble.h"

// This is from http://forums.getpebble.com/discussion/8280/displaying-the-value-of-a-floating-point
// because Pebble doesn't support %f in snprintf.
static char* floatToString(char* buffer, int bufferSize, double number) {
    char decimalBuffer[7];

    snprintf(buffer, bufferSize, "%d", (int)number);
    strcat(buffer, ".");

    snprintf(decimalBuffer, 7, "%04d", (int)((double)(number - (int)number) * (double)10000));
    strcat(buffer, decimalBuffer);

    return buffer;
}