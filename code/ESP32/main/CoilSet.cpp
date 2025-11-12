#include <Arduino.h>
#include "CoilSet.h"

CoilSet::CoilSet() : CDsize(0), CDbyteSize(0), CDbuffer(nullptr) {}

CoilSet::~CoilSet() {
   if (CDsize > 0) {
      if (CDbuffer) {
         delete CDbuffer;
         CDbuffer = nullptr;
         CDsize = 0;
      }
   }
}

void CoilSet::Init(uint16_t size, bool initValue) {
   // Limit the size to 2000 (Modbus rules)
   if (size > 2000)
      size = 2000;
   // Do we have a size?
   if (size) {
      // Calculate number of bytes needed
      CDbyteSize = byteIndex(size - 1) + 1;
      // Allocate and init buffer
      CDbuffer = new uint8_t[CDbyteSize];
      memset(CDbuffer, initValue ? 0xFF : 0, CDbyteSize);
      if (initValue) {
         CDbuffer[CDbyteSize - 1] &= CDfilter[bitIndex(size - 1)];
      }
      CDsize = size;
   }
}

// Comparison operators
bool CoilSet::operator==(const CoilSet &m) {
   // Self-compare is always true
   if (this == &m)
      return true;
   // Different sizes are never equal
   if (CDsize != m.CDsize)
      return false;
   // Compare the data
   if (CDsize > 0 && memcmp(CDbuffer, m.CDbuffer, CDbyteSize))
      return false;
   return true;
}

// Inequality: invert the result of the equality comparison
bool CoilSet::operator!=(const CoilSet &m) { return !(*this == m); }

// If used as vector<uint8_t>, return a complete slice
CoilSet::operator vector<uint8_t> const() {
   // Create new vector to return
   vector<uint8_t> retval;
   if (CDsize > 0) {
      // Copy over all buffer content
      retval.assign(CDbuffer, CDbuffer + CDbyteSize);
   }
   // return the copy (or an empty vector)
   return retval;
}

// slice: return a CoilSet object with coils shifted leftmost
// will return empty object if illegal parameters are detected
CoilSet CoilSet::slice(uint16_t start, uint16_t length) {
   CoilSet retval;

   // Any slice of an empty coilset is an empty coilset ;)
   if (CDsize == 0)
      return retval;

   // If start is beyond the available coils, return empty slice
   if (start > CDsize)
      return retval;

   // length default is all up to the end
   if (length == 0)
      length = CDsize - start;

   // Does the requested slice fit in the buffer?
   if ((start + length) <= CDsize) {
      // Yes, it does. Extend return object
      retval.Init(length);

      // Loop over all requested bits
      for (uint16_t i = start; i < start + length; ++i) {
         if (CDbuffer[byteIndex(i)] & (1 << bitIndex(i))) {
            retval.set(i - start, true);
         }
      }
   }
   return retval;
}

// operator[]: return value of a single coil
bool CoilSet::operator[](uint16_t index) const {
   if (index < CDsize) {
      return (CDbuffer[byteIndex(index)] & (1 << bitIndex(index))) ? true : false;
   }
   // Wrong parameter -> always return false
   return false;
}

// set functions to change coil value(s)
// Will return true if done, false if impossible (wrong address or data)
// set #1: alter one single coil
bool CoilSet::set(uint16_t index, bool value) {
   // Within coils?
   if (index < CDsize) {
      // Yes. Determine affected byte and bit therein
      uint16_t by = byteIndex(index);
      uint8_t mask = 1 << bitIndex(index);

      // Stamp out bit
      CDbuffer[by] &= ~mask;
      // If required, set it to 1 now
      if (value) {
         CDbuffer[by] |= mask;
      }
      return true;
   }
   // Wrong parameter -> always return false
   return false;
}

// set #2: alter a group of coils, overwriting it by the bits from vector newValue
bool CoilSet::set(uint16_t start, uint16_t length, vector<uint8_t> newValue) {
   // Does the vector contain enough data for the specified size?
   if (newValue.size() >= (size_t)(byteIndex(length - 1) + 1)) {
      // Yes, we safely may call set #3 with it
      return set(start, length, newValue.data());
   }
   return false;
}

// set #3: alter a group of coils, overwriting it by the bits from uint8_t buffer newValue
// **** Watch out! ****
// This may be a potential risk if newValue is pointing to an array shorter than required.
// Then heap data behind the array may be used to set coils!
bool CoilSet::set(uint16_t start, uint16_t length, uint8_t *newValue) {
   // Does the requested slice fit in the buffer?
   if (length && (start + length) <= CDsize) {
      // Yes, it does.
      // Prepare pointers to the source byte and the bit within
      uint8_t *cp = newValue;
      uint8_t bitPtr = 0;

      // Loop over all bits to be set
      for (uint16_t i = start; i < start + length; i++) {
         // Get affected byte
         uint8_t by = byteIndex(i);
         // Calculate single-bit mask in target byte
         uint8_t mask = 1 << bitIndex(i);
         // Stamp out bit
         CDbuffer[by] &= ~mask;
         // is source bit set?
         if (*cp & (1 << bitPtr)) {
            // Yes. Set it in target as well
            CDbuffer[by] |= mask;
         }
         // Advance source bit ptr
         bitPtr++;
         // Overflow?
         if (bitPtr >= 8) {
            // Yes. move pointers to first bit in next source byte
            bitPtr = 0;
            cp++;
         }
      }
      return true;
   }
   return false;
}
