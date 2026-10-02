#ifndef __psyz
extern void exit();
extern void puts(char*);
// setjmp
extern char* strcat(char*, char*);
extern char* strcpy(char*, char*);
extern int strlen(char*);
extern void* memcpy(unsigned char*, unsigned char*, int);
extern void* memset(unsigned char*, unsigned char, int);

/*
 * Returns a pseudo-random number from 0 to RAND_MAX (0x7FFF=32767).
 */
extern int rand(void);
extern void srand(unsigned int);
extern void* malloc(size_t // Size of memory block to be allocated
);
extern void free(void*);
int printf(char*, ...);
#endif

#ifdef __psyz
// libc2's itoa(n): the decimal digits of n in a static buffer, overwritten
// by the next call. No Psy-Q header declares it, and host C libraries that
// have an itoa() give it other parameters, so psyz names it psyz_itoa.
#define itoa psyz_itoa
char* psyz_itoa(int n);
#endif
