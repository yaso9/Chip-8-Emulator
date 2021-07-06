#include <chrono>
#include <fstream>
#include <memory>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "./Chip8.hpp"
#include "./Keypad.hpp"
#include "./Debugger.hpp"
#include "./main_menu.hpp"
#include "./threads/window.hpp"

int main(int argc, char **argv)
{
    sf::RenderWindow window(sf::VideoMode(Chip8::SCREEN_WIDTH * Chip8::PIXEL_SIZE, Chip8::SCREEN_HEIGHT * Chip8::PIXEL_SIZE + Keypad::KEYPAD_SIZE), "Chip 8 Emulator", sf::Style::Default ^ sf::Style::Resize);
    ImGui::SFML::Init(window);

    std::shared_ptr<Keypad> keypad = std::make_shared<Keypad>();
    std::shared_ptr<Chip8> chip8 = std::make_shared<Chip8>(keypad);
    std::shared_ptr<Debugger> debugger;
    keypad->add_key_press_handler(chip8);

    std::unique_ptr<std::thread> clock_thread;
    std::unique_ptr<std::thread> window_thread = create_window_thread(window, keypad, chip8, clock_thread, debugger);

    window_thread->join();
    if (clock_thread)
        clock_thread->join();

    return 0;
}
