#include <stdint.h>
#include <stdbool.h>

/*
hs = 2**(1/12)
a0 = 440 / 2**4
c0 = a0 * hs**-9
*/
#define C0_REF 16.351597831287407
#define  HALF_TONE_COEF 1.0594630943592953

typedef enum {
    NOTE_C  = 0,
    NOTE_CS = 1,
    NOTE_D  = 2,
    NOTE_DS = 3,
    NOTE_E  = 4,
    NOTE_F  = 5,
    NOTE_FS = 6,
    NOTE_G  = 7,
    NOTE_GS = 8,
    NOTE_A  = 9,
    NOTE_AS = 10,
    NOTE_B  = 11,
} NoteName;

typedef struct {
    NoteName name;
    int8_t octave;
    int8_t cents; // -50 to +50
    bool valid;
    float freq;
} Note;

extern char *noteNameStrings[12];

void freqToNote(double freq, Note *note);

