#ifndef VIEW_H
#define VIEW_H

#include <SFML/Graphics.hpp>
#include "model.h"
#include <utility>
#include <string>

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

// represents affine transformation matrix
struct Transform {
    float mat[6];
    Transform() {}

    static Transform id() {
        Transform t;
        t[0] = 1; t[1] = 0; t[2] = 0;
        t[3] = 0; t[4] = 1; t[5] = 0;
        return t;
    }

    static Transform trans(float dx, float dy) {
        Transform t = Transform::id();
        t[2] = dx;
        t[5] = dy;
        return t;
    }

    static Transform scale(float sx, float sy) {
        Transform t = Transform::id();
        t[0] = sx;
        t[4] = sy;
        return t;
    }

    static Transform scale(float s) {
        return scale(s, s);
    }

    float& operator[](size_t index) {
        return mat[index];
    }

    float& dx() {
        return mat[2];
    }

    float& dy() {
        return mat[5];
    }

    float& sx() {
        return mat[0];
    }

    float& sy() {
        return mat[4];
    }

    float det() const {
        return mat[0] * mat[4] - mat[1] * mat[3];
    }

    Transform inv() const {
        Transform t;
        float detInv = 1.f / det();
        t[0] = mat[4] * detInv;
        t[1] = (-mat[1]) * detInv;
        t[2] = (mat[1] * mat[5] - mat[2] * mat[4]) * detInv;
        t[3] = -mat[3] * detInv;
        t[4] = -mat[0] * detInv;
        t[5] = (mat[2] * mat[3] - mat[0] * mat[5]) * detInv;
        return t;
    }

    Transform operator*(Transform other) {
        Transform t;
        t[0] = mat[0] * other[0] + mat[1] * other[3];
        t[1] = mat[0] * other[1] + mat[1] * other[4];
        t[2] = mat[0] * other[2] + mat[1] * other[5] + mat[2];
        t[3] = mat[3] * other[0] + mat[4] * other[3];
        t[4] = mat[3] * other[1] + mat[4] * other[4];
        t[5] = mat[3] * other[2] + mat[4] * other[5] + mat[5];
        return t;
    }

    Transform scale() const {
        return Transform::scale(mat[0], mat[4]);
    }

    Pos operator*(Pos pos) const {
        return { mat[0] * pos.x + mat[1] * pos.y + mat[2], mat[3] * pos.x + mat[4] * pos.y + mat[5] };
    }
};

struct MouseState {
    bool mouse_down;
    bool just_changed;
    Pos pos;
    bool mouse_lost;
};

struct MouseRegion {
    Pos pos;
    Pos dim;
    MouseRegion(Pos pos = Pos(), Pos dim = Pos()) : pos(pos), dim(dim) {}
    sf::Rect<float> boundingBox(Transform tf);
    bool mouseTouching(const MouseState& mouse, Transform tf);
};

struct TextBox : public MouseRegion {
    int font_size;
    float value;
    static sf::Font font;
    static void init(sf::Font font);
    TextBox(Pos pos = Pos(), Pos dim = Pos(), int font_size = 12, float init_value = 0.f) : MouseRegion(pos, dim), font_size(font_size), value(init_value) {}
    void draw(sf::RenderWindow& window, Transform tf);
};

struct ExecBlockView {
    static sf::Font font;
    static void init(sf::Font font);

    const ExecBlock block;
    std::string label;

    MouseRegion task_mr;
    MouseRegion core_mr;

    ExecBlockView(ExecBlock block) : block(block) {
        label = std::to_string(block.task_id) + "," + std::to_string(block.job_id);
    }

    float getX() const;
    float getY(bool task_based) const;
    float getWidth() const;
    float getHeight() const;
    Pos getPos(bool task_based, int block_stretch) const;
    Pos getDim(int block_stretch) const;
    bool mouseTouching(const MouseState& mouse, Transform tf, Transform core_tracks_offset, int block_stretch); // return true if mouse hovered over
    void draw(sf::RenderWindow& window, Transform tf, int block_stretch, bool task_based, bool focused) const;
};

struct View {
    static sf::Font font;
    static void init();
    
    sf::RenderWindow window;
    Transform tf;
    std::vector<std::vector<ExecBlockView>> blocks;
    int block_stretch = 1;

    View() {}

    // opens window
    void open(unsigned int width, unsigned int height);

    // closes window
    void close();

    // returns true if window open false otherwise
    bool isOpen();

    // updates display
    void update(Model& model, const MouseState& mouse, float fps);
};

#endif