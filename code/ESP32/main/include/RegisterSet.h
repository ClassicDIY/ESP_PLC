#pragma once

#include <vector>
#include <cstdint>

using std::vector;

// RegisterSet: representing Modbus register values
class RegisterSet
{
public:
    // Maximum size is 250 registers
    RegisterSet();

    // Destructor: take care of cleaning up
    ~RegisterSet();

    void Init(uint16_t size, uint16_t initValue = 0);

    // Comparison operators
    bool operator==(const RegisterSet &m);
    bool operator!=(const RegisterSet &m);

    // If used as vector<uint16_t>, return the complete set
    operator vector<uint16_t> const();

    // slice: return a new RegisterSet object with registers shifted leftmost
    // will return empty set if illegal parameters are detected
    // Default start is first register, default length all to the end
    RegisterSet slice(uint16_t start = 0, uint16_t length = 0);

    // operator[]: return value of a single register
    uint16_t operator[](uint16_t index) const;

    // Set functions to change register value(s)
    // Will return true if done, false if impossible (wrong address or data)

    // set #1: alter one single register
    bool set(uint16_t index, uint16_t value);

    // set #2: alter a group of registers, overwriting it by the uint16_t from newValue
    bool set(uint16_t index, uint16_t length, vector<uint16_t> newValue);

    // set #3: alter a group of registers, overwriting it by the array of words
    bool set(uint16_t index, uint16_t length, uint16_t *newValue);

     // set #4: alter a group of registers, overwriting it by the array of byte
    bool set(uint16_t index, uint16_t length, uint8_t *data);

    // get size in registers
    inline uint16_t size() const { return RDsize; }
    inline bool empty() const { return (RDsize > 0) ? true : false; }
    uint16_t *data();

// protected:
    uint16_t RDsize = 0; // Size of the RegisterSet store in bits
    uint16_t *RDbuffer; // Pointer to bit storage
};
