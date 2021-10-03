#include <stdlib.h>

#include "random.h"

// These functions are to enable us to easily switch between different rngs
// for different platforms. Probably/possibly use #ifdef #elsif ...

int randomi(int upper) {
  return arc4random_uniform(upper);
}

double uniform() {
  return (double)arc4random()/UINT32_MAX;
}
