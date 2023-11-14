#ifndef VIEW_H
#define VIEW_H

#include <SFML/Graphics.hpp>
#include "model.h"
#include <utility>

// represents offset then scale
struct Transform {
    float dx;
    float dy;
    float scale;
    Transform(float dx = 0.f, float dy = 0.f, float scale = 1.f) : dx(dx), dy(dy), scale(scale) {}
};

struct Pos {
    float x;
    float y;

    Pos(float x = 0.f, float y = 0.f) : x(x), y(y) {}

    Pos(sf::Vector2f v) : x(v.x), y(v.y) {}
    Pos(sf::Vector2i v) : x(v.x), y(v.y) {}
    Pos(sf::Vector2u v) : x(v.x), y(v.y) {}

    sf::Vector2f operator*() const {
        return {x, y};
    }

    Pos operator*(Transform tf) const {
        return {(x + tf.dx) * tf.scale, (y + tf.dy) * tf.scale};
    }

    Pos operator/(Transform tf) const {
        return {x / tf.scale - tf.dx, y / tf.scale - tf.dy};
    }

    Pos operator*(float scale) const {
        return {x * scale, y * scale};
    }

    Pos operator/(float scale) const {
        return {x / scale, y / scale};
    }

    Pos operator+(Pos pos) const {
        return {x + pos.x, y + pos.y};
    }

    Pos operator-(Pos pos) const {
        return {x - pos.x, y - pos.y};
    }
};

struct ExecBlockView {
    const ExecBlock block;

    ExecBlockView(ExecBlock block) : block(block) {}

    float getX() const;
    float getY(bool task_based) const;
    float getWidth() const;
    float getHeight() const;
    Pos getPos(bool task_based) const;
    Pos getDim() const;
    void draw(sf::RenderWindow& window, Transform tf, bool task_based) const;
};

struct View {
    sf::RenderWindow window;
    Transform tf;
    std::vector<ExecBlockView> blocks;

    View() {}

    // opens window
    void open(unsigned int width, unsigned int height);

    // closes window
    void close();

    // returns true if window open false otherwise
    bool isOpen();

    // updates display
    void update(Model& model);
};

#endif