/*
 * BPM/cell controller communication
 */

static int _fofbIndex;

void
cellCommSetFOFB(int fofbIndex)
{
    _fofbIndex = fofbIndex;
}

int
cellCommGetFOFB(void)
{
    return _fofbIndex;
}
