#include "note.h"

#include <math.h>

char *noteNameStrings[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void freqToNote(double freq, Note *note) {
    note->freq = freq;

    if (freq < C0_REF) {
        note->cents = 0;
        note->name = 0;
        note->octave = 0;
        note->valid = false;
        return;
    }

    note->valid = true;
    double normalized = freq / C0_REF; // TODO optimization using log subtraction formula
    double halftones = log(normalized) / log(HALF_TONE_COEF);
    int halftonesRound = round(halftones);
    note->cents = (halftones - halftonesRound) * 100.0;
    note->name = halftonesRound % 12;
    note->octave = halftonesRound / 12;
}
