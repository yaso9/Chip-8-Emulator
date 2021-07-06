#pragma once

#include <array>

#include "./types.hpp"

struct Registers
{
public:
    std::array<reg_t, 0xF> general_regs{};
    addr_t addr_reg = 0;
    reg_t delay_reg = 0;
    reg_t sound_reg = 0;
    addr_t pc_reg = 0x200;
};
