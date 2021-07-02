#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <SFML/Graphics.hpp>

#include "./Chip8.hpp"

int main(int argc, char **argv)
{
    // Read the program
    std::ifstream program_stream("../rom", std::ios::binary);
    program_stream.seekg(0, std::ios::end);
    size_t program_size = program_stream.tellg();
    std::unique_ptr<byte[]> program = std::make_unique<byte[]>(program_size);
    program_stream.seekg(0, std::ios::beg);
    program_stream.read(reinterpret_cast<char *>(program.get()), program_size);

    sf::RenderWindow window(sf::VideoMode(Chip8::SCREEN_WIDTH * Chip8::PIXEL_SIZE, Chip8::SCREEN_HEIGHT * Chip8::PIXEL_SIZE), "Chip 8 Emulator", sf::Style::Default ^ sf::Style::Resize);

    Chip8 chip8(program, program_size);
    sf::Sprite display;
    sf::Clock timer;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Clock the CPU
        chip8.clock();

        // Draw the window
        window.clear(sf::Color::Black);
        window.draw(chip8);
        window.display();

        // Wait until the next clock
        std::this_thread::sleep_for(
            std::chrono::microseconds(
                (Chip8::TIME_BETWEEN_CLOCKS - timer.getElapsedTime()).asMicroseconds()));
        timer.restart();
    }

    return 0;
}
