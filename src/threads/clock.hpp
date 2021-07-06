#pragma once

#include <memory>
#include <thread>
#include <SFML/Graphics.hpp>

#include "../Chip8.hpp"

std::unique_ptr<std::thread> create_clock_thread(const sf::RenderWindow &window, const std::shared_ptr<Chip8> &chip8)
{
    // Create a thread to handle clocks
    return std::make_unique<std::thread>([&]
                                         {
                                             sf::Clock timer;
                                             while (window.isOpen())
                                             {
                                                 // Clock the CPU
                                                 chip8->clock();

                                                 // Wait until the next clock
                                                 std::this_thread::sleep_for(
                                                     std::chrono::microseconds(
                                                         (Chip8::TIME_BETWEEN_CLOCKS - timer.getElapsedTime()).asMicroseconds()));
                                                 timer.restart();
                                             }
                                         });
}