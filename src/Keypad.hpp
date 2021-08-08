#pragma once

#include <algorithm>
#include <SFML/Graphics.hpp>

#include "./Key.hpp"
#include "./KeyPressHandler.hpp"

class Keypad : public sf::Drawable
{
private:
    std::array<Key, 16> keys;
    std::shared_ptr<sf::Font> font;
    std::vector<std::shared_ptr<KeyPressHandler>> handlers;

public:
    // Some constants
    static constexpr const size_t KEYPAD_SIZE = 640;
    static constexpr const std::array<char, 16> KEY_CHARACTER{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static constexpr const std::array<uint8_t, 16> KEY_DRAW_ORDER{1, 2, 3, 0xC, 4, 5, 6, 0xD, 7, 8, 9, 0xE, 0xA, 0, 0xB, 0xF};

    Keypad() : font(std::make_shared<sf::Font>())
    {
        font->loadFromFile("../fonts/PressStart2P-vaV7.ttf");

        // Create all the keys
        for (int idx = 0; idx < KEY_DRAW_ORDER.size(); idx++)
        {
            keys[KEY_DRAW_ORDER[idx]] = Key(font, KEY_CHARACTER[KEY_DRAW_ORDER[idx]], Key::KEY_SIZE * (idx % 4), Key::KEY_SIZE * (idx / 4) + 320);
        }
    }

    void add_key_press_handler(std::shared_ptr<KeyPressHandler> handler)
    {
        handlers.push_back(handler);
    }

    constexpr void handle_key_event(sf::Event event)
    {
        // Turn the key code into the key
        const uint8_t key = event.key.code <= 5 ? event.key.code + 0xA : (event.key.code >= 26 && event.key.code <= 26 + 9 ? event.key.code - 26 : 0xFF);

        // Ignore the key if we don't handle it
        if (key == 0xFF)
            return;

        // Modify the key state appropriately
        switch (event.type)
        {
        case sf::Event::KeyPressed:
            keys[key].set_down(true);
            break;
        case sf::Event::KeyReleased:
            keys[key].set_down(false);
            break;
        }

        // Notify the handlers
        for (const std::shared_ptr<KeyPressHandler> &handler : handlers)
        {
            handler->handle_key_press(key);
        }
    }

    [[nodiscard]] inline bool is_key_down(uint8_t key) const
    {
        return keys[key].get_down();
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        for (const Key &key : keys)
        {
            target.draw(key);
        }
    }
};