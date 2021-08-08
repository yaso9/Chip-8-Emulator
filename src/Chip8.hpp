#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <memory>
#include <mutex>
#include <stack>
#include <SFML/Graphics.hpp>

#include "./get_bits.hpp"
#include "./types.hpp"
#include "./font.hpp"
#include "./Registers.hpp"
#include "./Keypad.hpp"
#include "./KeyPressHandler.hpp"
#include "./Program.hpp"
#include "./Debugger.hpp"

class Chip8 : public sf::Drawable, public KeyPressHandler
{
public:
    // Some constants
    static constexpr const size_t SCREEN_WIDTH = 64;
    static constexpr const size_t SCREEN_HEIGHT = 32;
    static constexpr const size_t PIXEL_SIZE = 10;
    static const sf::Time TIME_BETWEEN_CLOCKS;
    static constexpr const uint8_t CLOCKS_BETWEEN_TIMER_DECREMENT = 10;

private:
    // Registers
    std::shared_ptr<Registers> registers = std::make_shared<Registers>();

    // On the Chip 8, the stack is only used to store return addresses on function calls
    // The program is unable to interact with the stack pointer aside from pushing the
    // return address when calling a function and popping on return
    // Instead of placing a stack in emulator memory and having a stack pointer register,
    // a stack outside of program memory can safely be used
    std::stack<addr_t> stack{};

    // Memory
    std::shared_ptr<std::array<byte, 0x1000>> memory = std::make_shared<std::array<byte, 0x1000>>();

    // The delay and sound registers, when non-zero, decrement at 50 hertz
    // This behavior is approximated by decrementing the registers at a set interval of clocks
    uint8_t clocks_since_timer_decrement;

    // The keypad to get input from
    const std::shared_ptr<Keypad> keypad;

    // A texture for the pixels drawn on the screen
    sf::Texture pixel_texture;

    // A linked list of sprites on the screen
    std::list<sf::Sprite> sprites{};

    // This mutex is used to make sure a sprite isn't being drawn while the screen is being updated
    mutable std::mutex sprites_mutex;

    // These are used to handle waiting until a key is pressed for the Fx0A instruction
    std::mutex key_press_mtx;
    std::condition_variable key_press_cv;
    uint8_t key_pressed;

    // This handles debugging
    std::shared_ptr<Debugger> debugger;

    // Draw a chip8 sprite on the display
    // Returns true if there was a collision
    bool drawSprite(addr_t sprite_addr, uint8_t x, uint8_t y, uint8_t n)
    {
        std::unique_lock<std::mutex> lock(sprites_mutex);
        bool intersect = false;

        for (addr_t i = sprite_addr; i < sprite_addr + n; i++)
        {
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                if (get_bits((*memory)[i], 7 - bit, 1) == 1)
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
    // 0 out all the registers except for the program counter
    Chip8(std::shared_ptr<Keypad> keypad) : keypad(keypad)
    {
        // Copy the font to the beginning of memory
        std::copy(font.begin(), font.end(), memory->begin());

        // Create and initialize the pixel_texture
        sf::RenderTexture pixel_render;
        pixel_render.create(PIXEL_SIZE, PIXEL_SIZE);
        pixel_render.clear();
        sf::RectangleShape pixel_shape(sf::Vector2f(PIXEL_SIZE, PIXEL_SIZE));
        pixel_shape.setFillColor(sf::Color::White);
        pixel_render.draw(pixel_shape);
        pixel_texture = pixel_render.getTexture();
    }

    void attach_debugger(std::shared_ptr<Debugger> &debugger, bool break_next)
    {
        this->debugger = debugger;
        debugger->attach(memory, registers, break_next);
    }

    void load_program(Program *program)
    {
        // Download the program and copy it into memory starting at 0x200
        program->get_program();
        std::copy(program->program.begin(), program->program.end(), memory->begin() + 0x200);
    }

    virtual void handle_key_press(uint8_t key)
    {
        std::unique_lock<std::mutex> lock(key_press_mtx);
        key_pressed = key;
        key_press_cv.notify_all();
    }

    // Execute a clock of the Chip 8
    void clock()
    {
        if (debugger)
            debugger->on_clock();

        // Decrement the timer if there have been enough clocks since the last time it was decremented
        if (clocks_since_timer_decrement >= CLOCKS_BETWEEN_TIMER_DECREMENT)
        {
            if (registers->delay_reg > 0)
                registers->delay_reg -= 1;

            if (registers->sound_reg > 0)
                registers->sound_reg -= 1;

            clocks_since_timer_decrement = 0;
        }
        else
        {
            clocks_since_timer_decrement += 1;
        }

        // Get the instruction
        const inst_t instruction = ((*memory)[registers->pc_reg] << 8) + (*memory)[registers->pc_reg + 1];

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
                registers->pc_reg = stack.top();
                stack.pop();
                break;
            default:
                assert(("Invalid instruction", false));
                break;
            }
            break;
        case 0x1:
            // 1nnn - JP addr
            registers->pc_reg = nnn - 2;
            break;
        case 0x2:
            // 2nnn - CALL addr
            stack.push(registers->pc_reg);
            registers->pc_reg = nnn;
            break;
        case 0x3:
            // 3xkk - SE Vx, byte
            if (registers->general_regs[x] == kk)
                registers->pc_reg += 2;
            break;
        case 0x4:
            // 4xkk - SNE Vx, byte
            if (registers->general_regs[x] != kk)
                registers->pc_reg += 2;
            break;
        case 0x5:
            // 5xy0 - SE Vx, Vy
            if (registers->general_regs[x] == registers->general_regs[y])
                registers->pc_reg += 2;
            break;
        case 0x6:
            // 6xkk - LD Vx, byte
            registers->general_regs[x] = kk;
            break;
        case 0x7:
            // 7xkk - ADD Vx, byte
            registers->general_regs[x] += kk;
            break;
        case 0x8:
            switch (n)
            {
            case 0x0:
                // 8xy0 - LD Vx, Vy
                registers->general_regs[x] = registers->general_regs[y];
                break;
            case 0x1:
                // 8xy1 - OR Vx, Vy
                registers->general_regs[x] |= registers->general_regs[y];
                break;
            case 0x2:
                // 8xy2 - AND Vx, Vy
                registers->general_regs[x] &= registers->general_regs[y];
                break;
            case 0x3:
                // 8xy3 - XOR Vx, Vy
                registers->general_regs[x] ^= registers->general_regs[y];
                break;
            case 0x4:
                // 8xy4 - ADD Vx, Vy
                {
                    const uint16_t sum = static_cast<uint16_t>(registers->general_regs[x]) + static_cast<uint16_t>(registers->general_regs[y]);
                    registers->general_regs[0xF] = sum > 0xFF ? 1 : 0;
                    registers->general_regs[x] = sum;
                }
                break;
            case 0x5:
                // 8xy5 - SUB Vx, Vy
                registers->general_regs[0xF] = registers->general_regs[x] >= registers->general_regs[y] ? 1 : 0;
                registers->general_regs[x] -= registers->general_regs[y];
                break;
            case 0x6:
                // 8xy6 - SHR Vx {, Vy}
                registers->general_regs[0xF] = get_bits(registers->general_regs[y], 0, 1);
                registers->general_regs[x] = registers->general_regs[y] >> 1;
                break;
            case 0x7:
                // 8xy7 - SUBN Vx, Vy
                registers->general_regs[0xF] = registers->general_regs[y] >= registers->general_regs[x] ? 1 : 0;
                registers->general_regs[x] = registers->general_regs[y] - registers->general_regs[x];
                break;
            case 0xE:
                // 8xyE - SHL Vx {, Vy}
                registers->general_regs[0xF] = get_bits(registers->general_regs[y], 7, 1);
                registers->general_regs[x] = registers->general_regs[y] << 1;
                break;
            default:
                assert(("Invalid instruction", false));
                break;
            }
            break;
        case 0x9:
            // 9xy0 - SNE Vx, Vy
            if (registers->general_regs[x] != registers->general_regs[y])
                registers->pc_reg += 2;
            break;
        case 0xA:
            // Annn - LD I, addr
            registers->addr_reg = nnn;
            break;
        case 0xB:
            // Bnnn - JP V0, addr
            registers->pc_reg = nnn + registers->general_regs[0] - 2;
            break;
        case 0xC:
            // Cxkk - RND Vx, byte
            registers->general_regs[x] = (std::rand() % 0x100) & kk;
            break;
        case 0xD:
            // Dxyn - DRW Vx, Vy, nibble
            registers->general_regs[0xF] = drawSprite(registers->addr_reg, registers->general_regs[x], registers->general_regs[y], n) ? 1 : 0;
            break;
        case 0xE:
            switch (kk)
            {
            case 0x9E:
                // Ex9E - SKP Vx
                if (keypad->is_key_down(registers->general_regs[x]))
                    registers->pc_reg += 2;
                break;
            case 0xA1:
                // ExA1 - SKNP Vx
                if (!keypad->is_key_down(registers->general_regs[x]))
                    registers->pc_reg += 2;
                break;
            default:
                assert(("Invalid instruction", false));
                break;
            }
            break;
        case 0xF:
            switch (kk)
            {
            case 0x07:
                // Fx07 - LD Vx, DT
                registers->general_regs[x] = registers->delay_reg;
                break;
            case 0x0A:
                // Fx0A - LD Vx, K
                {
                    key_pressed = 0;
                    std::unique_lock<std::mutex> lock(key_press_mtx);
                    while (key_pressed == 0)
                        key_press_cv.wait(lock);
                    registers->general_regs[x] = key_pressed;
                }
                break;
            case 0x15:
                // Fx15 - LD DT, Vx
                registers->delay_reg = registers->general_regs[x];
                break;
            case 0x18:
                // Fx18 - LD ST, Vx
                registers->sound_reg = registers->general_regs[x];
                break;
            case 0x1E:
                // Fx1E - ADD I, Vx
                registers->addr_reg += registers->general_regs[x];
                break;
            case 0x29:
                // Fx29 - LD F, Vx
                registers->addr_reg = registers->general_regs[x] * 5;
                break;
            case 0x33:
                // Fx33 - LD B, Vx
                (*memory)[registers->addr_reg] = registers->general_regs[x] / 100;
                (*memory)[registers->addr_reg + 1] = (registers->general_regs[x] / 10) % 10;
                (*memory)[registers->addr_reg + 2] = registers->general_regs[x] % 10;
                break;
            case 0x55:
                // Fx55 - LD [I], Vx
                std::copy(registers->general_regs.begin(), registers->general_regs.begin() + x + 1, memory->begin() + registers->addr_reg);
                registers->addr_reg += x + 1;
                break;
            case 0x65:
                // Fx65 - LD Vx, [I]
                std::copy(memory->begin() + registers->addr_reg, memory->begin() + registers->addr_reg + x + 1, registers->general_regs.begin());
                registers->addr_reg += x + 1;
                break;
            default:
                assert(("Invalid instruction", false));
                break;
            }
            break;
        default:
            assert(("Invalid instruction", false));
            break;
        }

        // Increment the program counter
        registers->pc_reg += 2;
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        std::unique_lock<std::mutex> lock(sprites_mutex);
        for (const sf::Sprite &sprite : sprites)
        {
            target.draw(sprite);
        }
    }
};

const sf::Time Chip8::TIME_BETWEEN_CLOCKS = sf::milliseconds(2); // 500 hz clock