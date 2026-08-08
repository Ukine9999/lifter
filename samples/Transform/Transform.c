#include <stdint.h>

static uint64_t RotateLeft64(uint64_t value, uint32_t amount)
{
    return (value << amount) | (value >> (64u - amount));
}

__declspec(dllexport) __declspec(noinline) uint64_t TransformCore(const void* input, uint64_t length,
                                                                  uint64_t reservedOne, uint64_t reservedTwo)
{
    const uint8_t* bytes = input;
    uint64_t state = UINT64_C(0xcbf29ce484222325);
    (void)reservedOne;
    (void)reservedTwo;

    for (uint64_t index = 0; index < length; ++index)
    {
        const uint8_t byteValue = bytes[index];
        state ^= byteValue;
        state *= UINT64_C(0x100000001b3);
        state = RotateLeft64(state, (uint32_t)(byteValue & 31u) + 1u);

        if ((byteValue & 1u) != 0)
            state += (uint64_t)byteValue * UINT64_C(0x9e3779b97f4a7c15);
        else
            state ^= (~state) >> 7u;
    }

    state ^= state >> 33u;
    state *= UINT64_C(0xff51afd7ed558ccd);
    state ^= state >> 29u;
    return state;
}
