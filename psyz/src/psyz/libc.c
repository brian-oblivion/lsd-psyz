#include <psyz.h>
#include <rand.h>

// libc2's generator: a 32-bit linear congruential generator whose bits 16
// to 30 are the result. srand() sets the state. libc2 keeps it in .sbss, so
// it starts at 0, not at the 1 the manual gives.
static unsigned int rand_next = 0;

int rand(void) {
    rand_next = rand_next * 1103515245 + 12345;
    return (rand_next >> 16) & RAND_MAX;
}

void srand(unsigned int seed) { rand_next = seed; }
