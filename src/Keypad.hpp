#pragma once

#include <cassert>
#include <SFML/Graphics.hpp>

#include "./Chip8.hpp"

class Keypad : public sf::Drawable
{
private:
    // Hold the state (up/down) of each key
    enum class KeyState
    {
        Down,
        Up,
    };
    std::array<KeyState, 16> key_states;

    // The font used on the keys
    sf::Font font;

public:
    // Some constants
    static constexpr const size_t KEYPAD_SIZE = 640;
    static constexpr const size_t KEY_SIZE = KEYPAD_SIZE / 4;
    static constexpr const size_t KEY_FONT_SIZE = 120;
    static constexpr const size_t KEY_BORDER_SIZE = 10;
    static constexpr const std::array<char, 16> KEY_CHARACTER{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static constexpr const std::array<uint8_t, 16> KEY_DRAW_ORDER{1, 2, 3, 0xC, 4, 5, 6, 0xD, 7, 8, 9, 0xE, 0xA, 0, 0xB, 0xF};

    Keypad()
    {
        assert(font.loadFromFile("../fonts/PressStart2P-vaV7.ttf"));

        // Start with all the keys in the up position
        key_states.fill(KeyState::Up);
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
            key_states[key] = KeyState::Down;
            break;
        case sf::Event::KeyReleased:
            key_states[key] = KeyState::Up;
            break;
        }
    }

    [[nodiscard]] inline constexpr bool is_key_down(uint8_t key) const
    {
        return key_states[key] == KeyState::Down;
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        // Initialize the rectangle around each key
        sf::RectangleShape key_rect;
        key_rect.setOutlineColor(sf::Color::Cyan);
        key_rect.setOutlineThickness(KEY_BORDER_SIZE);
        key_rect.setSize(sf::Vector2f(KEY_SIZE - KEY_BORDER_SIZE * 2, KEY_SIZE - KEY_BORDER_SIZE * 2));

        // Initialize the text in each key
        sf::Text text;
        text.setFont(font);
        text.setFillColor(sf::Color::Red);
        text.setCharacterSize(KEY_FONT_SIZE);

        for (uint8_t idx = 0; idx < KEY_CHARACTER.size(); idx++)
        {
            // Set the color and position of this key's rectangle and draw it
            key_rect.setFillColor(is_key_down(KEY_DRAW_ORDER[idx]) ? sf::Color::Green : sf::Color::Yellow);
            key_rect.setPosition(KEY_SIZE * (idx % 4) + KEY_BORDER_SIZE, 320 + (idx / 4) * KEY_SIZE + KEY_BORDER_SIZE);
            target.draw(key_rect);

            // Set the position of this key's text and draw it
            text.setString(KEY_CHARACTER[KEY_DRAW_ORDER[idx]]);
            sf::FloatRect bounds = text.getGlobalBounds();
            text.setPosition(KEY_SIZE * (idx % 4) + (KEY_SIZE - bounds.width) / 2, 320 + (idx / 4) * KEY_SIZE + (KEY_SIZE - bounds.height) / 2);
            target.draw(text);
        }
    }
};