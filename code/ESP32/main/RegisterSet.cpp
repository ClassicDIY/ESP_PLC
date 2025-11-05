#include "Log.h"
#include "Defines.h"
#include "RegisterSet.h"

// Constructor:
// Maximum size is 255
RegisterSet::RegisterSet() : RDsize(0), RDbuffer(nullptr)
{
}

// Destructor: take care of cleaning up
RegisterSet::~RegisterSet()
{
  if (RDsize > 0)
  {
    if (RDbuffer != nullptr)
    {
      delete RDbuffer;
      RDbuffer = nullptr;
      RDsize = 0;
    }
  }
}

void RegisterSet::Init(uint16_t size, uint16_t initValue)
{
  // Limit the size to 255
  if (size > 255)
    size = 255;
  if (size)
  {
    // Allocate and init buffer
    RDbuffer = new uint16_t[size];
    for (int i = 0; i < size; i++)
    {
      RDbuffer[i] = initValue;
    }
    RDsize = size;
  }
}

// Comparison operators
bool RegisterSet::operator==(const RegisterSet &m)
{
  // Self-compare is always true
  if (this == &m)
    return true;
  // Different sizes are never equal
  if (RDsize != m.RDsize)
    return false;
  // Compare the data
  if (RDsize > 0 && memcmp(RDbuffer, m.RDbuffer, RDsize))
    return false;
  return true;
}

// If used as vector<uint16_t>, return a complete slice
RegisterSet::operator vector<uint16_t> const()
{
  // Create new vector to return
  vector<uint16_t> retval;
  if (RDsize > 0)
  {
    // Copy over all buffer content
    retval.assign(RDbuffer, RDbuffer + RDsize);
  }
  // return the copy (or an empty vector)
  return retval;
}

uint16_t *RegisterSet::data()
{
  return RDbuffer;
}

// slice: return a subset RegisterSet object
// will return empty object if illegal parameters are detected
RegisterSet RegisterSet::slice(uint16_t start, uint16_t length)
{
  RegisterSet retval;

  // Any slice of an empty registerset is an empty registerset ;)
  if (RDsize == 0)
    return retval;

  // If start is beyond the available registers, return empty slice
  if (start > RDsize)
    return retval;

  // length default is all up to the end
  if (length == 0)
    length = RDsize - start;

  // Does the requested slice fit in the buffer?
  if ((start + length) <= RDsize)
  {
    // Yes, it does. Extend return object
    retval.Init(length);

    // Loop over all requested registers
    for (uint16_t i = start; i < start + length; ++i)
    {
      retval.set(i - start, RDbuffer[i]);
    }
  }
  return retval;
}

// operator[]: return value of a single register
uint16_t RegisterSet::operator[](uint16_t index) const
{
  if (index < RDsize)
  {
    return RDbuffer[index];
  }
  // Wrong parameter -> always return 0
  return 0;
}

// set #1: alter one single register
bool RegisterSet::set(uint16_t index, uint16_t value)
{
  // Within coils?
  if (index < RDsize)
  {
    RDbuffer[index] = value;
    return true;
  }
  // Wrong parameter -> always return false
  return false;
}

// set #2: alter a group of registers
bool RegisterSet::set(uint16_t start, uint16_t length, vector<uint16_t> newValue)
{
  // Does the vector contain enough data for the specified size?
  if (newValue.size() >= (size_t)RDsize)
  {
    // Yes, we safely may call set #3 with it
    return set(start, length, newValue.data());
  }
  return false;
}

// set #3: alter a group of registers
bool RegisterSet::set(uint16_t start, uint16_t length, uint16_t *newValue)
{
  // Does the requested slice fit in the buffer?
  if ((start + length) <= RDsize)
  {
    // Yes, it does.
    // Prepare pointers to the source byte and the bit within
    uint16_t *cp = newValue;
    // Loop over all bits to be set
    for (uint16_t i = start; i < start + length; i++)
    {
      RDbuffer[i] = *cp++;
    }
    return true;
  }
  return false;
}
