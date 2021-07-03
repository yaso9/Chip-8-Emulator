#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <memory>
#include <stack>
#include <SFML/Graphics.hpp>

#include "./types.hpp"
#include "./font.hpp"

class Chip8 : public sf::Drawable
{
private:
    // Registers
    std::array<reg_t, 0xF> general_regs{};
    addr_t addr_reg = 0;
    reg_t delay_reg = 0;
    reg_t sound_reg = 0;
    addr_t pc_reg = 0x200;

    // On the Chip 8, the stack is only used to store return addresses on function calls
    // The program is unable to interact with the stack pointer aside from pushing the
    // return address when calling a function and popping on return
    // Instead of placing a stack in emulator memory and having a stack pointer register,
    // a stack outside of program memory can safely be used
    std::stack<addr_t> stack{};

    // Memory
    std::array<byte, 0xFFF> memory{};

    // The delay and sound registers, when non-zero, decrement at 50 hertz
    // This behavior is approximated by decrementing the registers at a set interval of clocks
    uint8_t clocks_since_timer_decrement;

    // A texture for the pixels drawn on the screen
    sf::Texture pixel_texture;

    // A linked list of all the sprites currently on screen, for collision detection
    std::list<sf::Sprite> sprites{};

    // Get the n bits of value after the first offset bits
    template <typename Type>
    [[nodiscard]] static constexpr inline Type get_bits(Type value, size_t offset, size_t n)
    {
        return (value >> offset) & (Type)(pow(2, n) - 1);
    }

    // Draw a chip8 sprite on the display
    // Returns true if there was a collision
    bool drawSprite(addr_t sprite_addr, uint8_t x, uint8_t y, uint8_t n)
    {
        // Chip 8 sprites are n bytes long
        // Each byte is a row
        // Each bit represents a pixel
        // We use a SFML sprite to represent each Chip 8 pixel

        bool intersect = false;
        for (addr_t i = sprite_addr; i < sprite_addr + n; i++)
        {
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                if (get_bits(memory[i], 7 - bit, 1) == 1)
                {
                    sf::Sprite pixel(pixel_texture);
                    pixel.setPosition(sf::Vector2f((x + bit) * PIXEL_SIZE, (y + i - sprite_addr) * PIXEL_SIZE));

                    bool pixel_intersect = false;
                    for (std::list<sf::Sprite>::iterator it = sprites.begin(); it != sprites.end(); it++)
                    {
                        if (pixel.getGlobalBounds().intersects(it->getGlobalBounds()))
                        {
                            pixel_intersect = true;
                            intersect = true;

                            sprites.erase(it);
                            it--;
                            break;
                        }
                    }

                    if (!pixel_intersect)
                        sprites.push_back(pixel);
                }
            }
        }

        return intersect;
    }

public:
    // Some constants
    static constexpr const size_t SCREEN_WIDTH = 64;
    static constexpr const size_t SCREEN_HEIGHT = 32;
    static constexpr const size_t PIXEL_SIZE = 10;
    static const sf::Time TIME_BETWEEN_CLOCKS;
    static constexpr const uint8_t CLOCKS_BETWEEN_TIMER_DECREMENT = 10;

    // 0 out all the registers except for the program counter
    // The program is loaded in memory at 0x200
    Chip8(std::unique_ptr<byte[]> &program, size_t program_size)
    {
        // Copy the font to the beginning of memory
        std::copy(font.begin(), font.end(), memory.begin());

        // Copy the program to 0x200
        std::copy(program.get(), program.get() + program_size, memory.begin() + 0x200);

        // Create and initialize the pixel_texture
        sf::RenderTexture pixel_render;
        pixel_render.create(PIXEL_SIZE, PIXEL_SIZE);
        pixel_render.clear();
        sf::RectangleShape pixel_shape(sf::Vector2f(PIXEL_SIZE, PIXEL_SIZE));
        pixel_shape.setFillColor(sf::Color::White);
        pixel_render.draw(pixel_shape);
        pixel_texture = pixel_render.getTexture();
    }

    // Execute a clock of the Chip 8
    constexpr void clock()
    {
        // Decrement the timer if there have been enough clocks since the last time it was decremented
        if (clocks_since_timer_decrement >= CLOCKS_BETWEEN_TIMER_DECREMENT)
        {
            if (delay_reg > 0)
                delay_reg -= 1;

            if (sound_reg > 0)
                sound_reg -= 1;

            clocks_since_timer_decrement = 0;
        }
        else
        {
            clocks_since_timer_decrement += 1;
        }

        // Get the instruction
        const inst_t instruction = (memory[pc_reg] << 8) + memory[pc_reg + 1];

        // nnn - A 12-bit value, the lowest 12 bits of the instruction
        const addr_t nnn = get_bits(instruction, 0, 12);
        // n - A 4-bit value, the lowest 4 bits of the instruction
        const uint8_t n = get_bits(instruction, 0, 4);
        // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
        const uint8_t x = get_bits(instruction, 8, 4);
        // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
        const uint8_t y = get_bits(instruction, 4, 4);
        // kk - An 8-bit value, the lowest 8 bits of the instruction
        const uint8_t kk = get_bits(instruction, 0, 8);

        // Switch on the most significant nibble of the instruction
        switch (get_bits(instruction, 12, 4))
        {
        case 0x0:
            switch (nnn)
            {
            case 0x0E0:
                // 00E0 - CLS
                sprites.clear();
                break;
            case 0x0EE:
                // 00EE - RET
                pc_reg = stack.top();
                stack.pop();
                break;
            }
            break;
        case 0x1:
            // 1nnn - JP addr
            pc_reg = nnn - 2;
            break;
        case 0x2:
            // 2nnn - CALL addr
            stack.push(pc_reg);
            pc_reg = nnn;
            break;
        case 0x3:
            // 3xkk - SE Vx, byte
            if (general_regs[x] == kk)
                pc_reg += 2;
            break;
        case 0x4:
            // 4xkk - SNE Vx, byte
            if (general_regs[x] != kk)
                pc_reg += 2;
            break;
        case 0x5:
            // 5xy0 - SE Vx, Vy
            if (general_regs[x] == general_regs[y])
                pc_reg += 2;
            break;
        case 0x6:
            // 6xkk - LD Vx, byte
            general_regs[x] = kk;
            break;
        case 0x7:
            // 7xkk - ADD Vx, byte
            general_regs[x] += kk;
            break;
        case 0x8:
            switch (n)
            {
            case 0x0:
                // 8xy0 - LD Vx, Vy
                general_regs[x] = general_regs[y];
                break;
            case 0x1:
                // 8xy1 - OR Vx, Vy
                general_regs[x] |= general_regs[y];
                break;
            case 0x2:
                // 8xy2 - AND Vx, Vy
                general_regs[x] &= general_regs[y];
                break;
            case 0x3:
                // 8xy3 - XOR Vx, Vy
                general_regs[x] ^= general_regs[y];
                break;
            case 0x4:
                // 8xy4 - ADD Vx, Vy
                {
                    const uint16_t sum = static_cast<uint16_t>(general_regs[x]) + static_cast<uint16_t>(general_regs[y]);
                    general_regs[0xF] = sum > 0xFF ? 1 : 0;
                    general_regs[x] = sum;
                }
                break;
            case 0x5:
                // 8xy5 - SUB Vx, Vy
                general_regs[0xF] = general_regs[x] > general_regs[y] ? 1 : 0;
                general_regs[x] -= general_regs[y];
                break;
            case 0x6:
                // 8xy6 - SHR Vx {, Vy}
                general_regs[0xF] = get_bits(general_regs[y], 0, 1);
                general_regs[x] = general_regs[y] >> 1;
                break;
            case 0x7:
                // 8xy7 - SUBN Vx, Vy
                general_regs[0xF] = general_regs[y] > general_regs[x] ? 1 : 0;
                general_regs[x] = general_regs[y] - general_regs[x];
                break;
            case 0xE:
                // 8xyE - SHL Vx {, Vy}
                general_regs[0xF] = get_bits(general_regs[y], 7, 1);
                general_regs[x] = general_regs[y] << 1;
                break;
            }
            break;
        case 0x9:
            // 9xy0 - SNE Vx, Vy
            if (general_regs[x] != general_regs[y])
                pc_reg += 2;
            break;
        case 0xA:
            // Annn - LD I, addr
            addr_reg = nnn;
            break;
        case 0xB:
            // Bnnn - JP V0, addr
            pc_reg = nnn + general_regs[0];
            break;
        case 0xC:
            // Cxkk - RND Vx, byte
            general_regs[x] = (std::rand() % 0x100) & kk;
            break;
        case 0xD:
            // Dxyn - DRW Vx, Vy, nibble
            general_regs[0xF] = drawSprite(addr_reg, general_regs[x], general_regs[y], n) ? 1 : 0;
            break;
        case 0xE:
            switch (kk)
            {
            case 0x9E:
                // Ex9E - SKP Vx
                // TODO: Skip next instruction if key with the value of Vx is pressed.
                break;
            case 0xA1:
                // ExA1 - SKNP Vx
                // TODO: Skip next instruction if key with the value of Vx is not pressed.
                break;
            }
            break;
        case 0xF:
            switch (kk)
            {
            case 0x07:
                // Fx07 - LD Vx, DT
                general_regs[x] = delay_reg;
                break;
            case 0x0A:
                // Fx0A - LD Vx, K
                // TODO:  Wait for a key press, store the value of the key in Vx.
                break;
            case 0x15:
                // Fx15 - LD DT, Vx
                delay_reg = general_regs[x];
                break;
            case 0x18:
                // Fx18 - LD ST, Vx
                sound_reg = general_regs[x];
                break;
            case 0x1E:
                // Fx1E - ADD I, Vx
                addr_reg += general_regs[x];
                break;
            case 0x29:
                // Fx29 - LD F, Vx
                addr_reg = general_regs[x] * 5;
                break;
            case 0x33:
                // Fx33 - LD B, Vx
                memory[addr_reg] = general_regs[x] / 100;
                memory[addr_reg + 1] = (general_regs[x] / 10) % 10;
                memory[addr_reg + 2] = general_regs[x] % 10;
                break;
            case 0x55:
                // Fx55 - LD [I], Vx
                std::copy(general_regs.begin(), general_regs.begin() + x + 1, memory.begin() + addr_reg);
                addr_reg += x + 1;
                break;
            case 0x65:
                // Fx65 - LD Vx, [I]
                std::copy(memory.begin() + addr_reg, memory.begin() + addr_reg + x + 1, general_regs.begin());
                addr_reg += x + 1;
                break;
            }
            break;
        }

        // Increment the program counter
        pc_reg += 2;
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        for (const sf::Sprite &sprite : sprites)
        {
            target.draw(sprite);
        }
    }
};

const sf::Time Chip8::TIME_BETWEEN_CLOCKS = sf::milliseconds(2); // 500 hz clock