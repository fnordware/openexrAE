

// The only way I can find to force __SSE2__ off is to use this
// file as a prefix header.  AE plug-ins were already using
// Carbon.h as a prefix, so it goes here too.

#include "Carbon.h"

#undef __SSE2__
