#pragma once

#include <memory>
#include <SFML/Graphics.hpp>

class Key : public sf::Drawable
{
private:
    // The rectangle around the key
    sf::RectangleShape rect;

    // The text on the key
    sf::Text text;

    // Whether the key is down
    bool is_down = false;

public:
    static constexpr const size_t KEY_SIZE = 160;
    static constexpr const size_t KEY_FONT_SIZE = 120;
    static constexpr const size_t KEY_BORDER_SIZE = 10;

    Key() {}

    Key(std::shared_ptr<sf::Font> font, char character, size_t x, size_t y)
    {
        rect.setFillColor(sf::Color::Yellow);
        rect.setOutlineColor(sf::Color::Cyan);
        rect.setOutlineThickness(KEY_BORDER_SIZE);
        rect.setSize(sf::Vector2f(KEY_SIZE - KEY_BORDER_SIZE * 2, KEY_SIZE - KEY_BORDER_SIZE * 2));
        rect.setPosition(x + KEY_BORDER_SIZE, y + KEY_BORDER_SIZE);

        text.setFont(*font);
        text.setFillColor(sf::Color::Red);
        text.setCharacterSize(KEY_FONT_SIZE);
        text.setString(character);
        sf::FloatRect bounds = text.getGlobalBounds();
        text.setPosition(x + (KEY_SIZE - bounds.width) / 2, y + (KEY_SIZE - bounds.height) / 2);
    }

    void set_down(bool is_down)
    {
        this->is_down = is_down;
        rect.setFillColor(is_down ? sf::Color::Green : sf::Color::Yellow);
    }

    [[nodiscard]] inline constexpr bool get_down() const
    {
        return is_down;
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        target.draw(rect);
        target.draw(text);
    }
};