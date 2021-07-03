#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <SFML/Graphics.hpp>

#include "./Chip8.hpp"
#include "./Keypad.hpp"

int main(int argc, char **argv)
{
    // Read the program
    std::ifstream program_stream("../rom", std::ios::binary);
    program_stream.seekg(0, std::ios::end);
    size_t program_size = program_stream.tellg();
    std::unique_ptr<byte[]> program = std::make_unique<byte[]>(program_size);
    program_stream.seekg(0, std::ios::beg);
    program_stream.read(reinterpret_cast<char *>(program.get()), program_size);

    sf::RenderWindow window(sf::VideoMode(Chip8::SCREEN_WIDTH * Chip8::PIXEL_SIZE, Chip8::SCREEN_HEIGHT * Chip8::PIXEL_SIZE + Keypad::KEYPAD_SIZE), "Chip 8 Emulator", sf::Style::Default ^ sf::Style::Resize);

    std::shared_ptr<Keypad> keypad = std::make_shared<Keypad>();
    Chip8 chip8(program, program_size, keypad);
    sf::Sprite display;
    sf::Clock timer;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
            case sf::Event::KeyReleased:
                keypad->handle_key_event(event);
                break;
            }
        }

        // Clock the CPU
        chip8.clock();

        // Draw the window
        window.clear(sf::Color::Black);
        window.draw(chip8);
        window.draw(*keypad);
        window.display();

        // Wait until the next clock
        std::this_thread::sleep_for(
            std::chrono::microseconds(
                (Chip8::TIME_BETWEEN_CLOCKS - timer.getElapsedTime()).asMicroseconds()));
        timer.restart();
    }

    return 0;
}
