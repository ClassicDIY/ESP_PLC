#pragma once
#include <vector>
#include <cstdint>

using std::vector;

// CoilSet: representing Modbus coil (=bit) values
class CoilSet {
 public:
   CoilSet();
   ~CoilSet();

   void Init(uint16_t size, bool initValue = false);

   bool operator==(const CoilSet &m);
   bool operator!=(const CoilSet &m);

   // If used as vector<uint8_t>, return the complete set
   operator vector<uint8_t> const();

   // slice: return a new CoilSet object with coils shifted leftmost
   // will return empty set if illegal parameters are detected
   // Default start is first coil, default length all to the end
   CoilSet slice(uint16_t start = 0, uint16_t length = 0);

   // operator[]: return value of a single coil
   bool operator[](uint16_t index) const;

   // Set functions to change coil value(s)
   // Will return true if done, false if impossible (wrong address or data)

   // set #1: alter one single coil
   bool set(uint16_t index, bool value);

   // set #2: alter a group of coils, overwriting it by the bits from newValue
   bool set(uint16_t index, uint16_t length, vector<uint8_t> newValue);

   // set #3: alter a group of coils, overwriting it by the bits from unit8_t buffer newValue
   bool set(uint16_t index, uint16_t length, uint8_t *newValue);

   // get size in coils
   inline uint16_t coils() const { return CDsize; }

   // Raw access to coil data buffer
   inline uint8_t *data() const { return CDbuffer; };
   inline uint8_t size() const { return CDbyteSize; };

   // Test if there are any coils in object
   inline bool empty() const { return (CDsize > 0) ? true : false; }
   inline operator bool() const { return empty(); }

 protected:
   // bit masks for bits left of a bit index in a byte
   const uint8_t CDfilter[8] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
   // Calculate byte index and bit index within that byte
   inline uint8_t byteIndex(uint16_t index) const { return index >> 3; }
   inline uint8_t bitIndex(uint16_t index) const { return index & 0x07; }

   uint16_t CDsize;    // Size of the CoilSet store in bits
   uint8_t CDbyteSize; // Size in bytes
   uint8_t *CDbuffer;  // Pointer to bit storage
};
