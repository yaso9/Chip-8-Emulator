#pragma once

#include <thread>

#include "../Chip8.hpp"
#include "../Keypad.hpp"
#include "../main_menu.hpp"

std::unique_ptr<std::thread> create_window_thread(sf::RenderWindow &window, const std::shared_ptr<Keypad> keypad, const std::shared_ptr<Chip8> chip8, std::unique_ptr<std::thread> &clock_thread, std::shared_ptr<Debugger> &debugger)
{
    // Create a thread to handle drawing the window and handling events
    window.setActive(false);
    return std::make_unique<std::thread>([&]
                                         {
                                             sf::Clock deltaClock;
                                             window.setActive(true);
                                             while (window.isOpen())
                                             {
                                                 sf::Event event;
                                                 while (window.pollEvent(event))
                                                 {
                                                     ImGui::SFML::ProcessEvent(event);

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

                                                 ImGui::SFML::Update(window, deltaClock.restart());

                                                 if (!clock_thread)
                                                     main_menu(window, chip8, clock_thread, debugger);
                                                 else if (debugger)
                                                     debugger->draw_debugger();

                                                 // Draw the window
                                                 window.clear(sf::Color::Black);
                                                 window.draw(*chip8);
                                                 window.draw(*keypad);
                                                 ImGui::SFML::Render(window);
                                                 window.display();
                                             }
                                         });
}
