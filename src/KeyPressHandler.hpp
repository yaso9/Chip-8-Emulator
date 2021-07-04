#pragma once

#include <cstdint>

class KeyPressHandler
{
public:
    virtual void handle_key_press(uint8_t key) = 0;
};