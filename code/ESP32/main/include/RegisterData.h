#pragma once

#include <vector>
#include <cstdint>

using std::vector;

// registerData: representing Modbus register values
class RegisterData
{
public:
    // Maximum size is 250 registers
    explicit RegisterData(uint16_t size = 0, uint16_t initValue = 0);

    // Destructor: take care of cleaning up
    ~RegisterData();

    // Assignment operator
    RegisterData& operator=(const RegisterData& m);

    // Copy constructor
    RegisterData(const RegisterData &m);

    // Comparison operators
    bool operator==(const RegisterData &m);
    bool operator!=(const RegisterData &m);

    // If used as vector<uint16_t>, return the complete set
    operator vector<uint16_t> const();

    // slice: return a new registerData object with registers shifted leftmost
    // will return empty set if illegal parameters are detected
    // Default start is first register, default length all to the end
    RegisterData slice(uint16_t start = 0, uint16_t length = 0);

    // operator[]: return value of a single register
    uint16_t operator[](uint16_t index) const;

    // Set functions to change register value(s)
    // Will return true if done, false if impossible (wrong address or data)

    // set #1: alter one single register
    bool set(uint16_t index, uint16_t value);

    // set #2: alter a group of registers, overwriting it by the uint16_t from newValue
    bool set(uint16_t index, uint16_t length, vector<uint16_t> newValue);

    // set #3: alter a group of registers, overwriting it by the bits from unit8_t buffer newValue
    bool set(uint16_t index, uint16_t length, uint16_t *newValue);

    // set #4: alter a group of registers, overwriting it by the registers in another registerData object
    // Setting stops when either target storage or source registers are exhausted
    bool set(uint16_t index, const RegisterData &c);

    // get size in registers
    inline uint16_t size() const { return RDsize; }
    inline bool empty() const { return (RDsize > 0) ? true : false; }
    uint16_t *data();

// protected:
    uint16_t RDsize;    // Size of the registerData store in bits
    uint16_t *RDbuffer; // Pointer to bit storage
};
