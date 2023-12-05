#ifndef VIEW_H
#define VIEW_H

#include <SFML/Graphics.hpp>
#include "model.h"
#include <utility>
#include <string>
#include <unordered_map>

const float MAX_ZOOM = 10000.f;
const float MIN_ZOOM = 0.5f;
const float START_ZOOM = 2.0f;

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
        t[4] = mat[0] * detInv;
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

class KeyState {
    bool key_down = false;
    int update_tick = -1;
    int type_tick = -1;
    static int tick;
    static std::unordered_map<sf::Keyboard::Key, KeyState> key_map;
public:
    static void pressKey(sf::Keyboard::Key key);
    static void releaseKey(sf::Keyboard::Key key);
    static void reset();
    static void stepTick();
    static bool keyPressed(sf::Keyboard::Key key); // returns whether key currently pressed
    static bool keyReleased(sf::Keyboard::Key key); // returns whether key currently not pressed
    static bool keyJustPressed(sf::Keyboard::Key key); // returns whether key just switched from unpressed to pressed
    static bool keyJustReleased(sf::Keyboard::Key key); // returns whether key just switched from pressed to unpressed
    static bool keyTyped(sf::Keyboard::Key key); // returns whether key just recieved key press event (repeats when key held)
};

struct MouseRegion {
    Pos pos;
    Pos dim;
    MouseRegion(Pos pos = Pos(), Pos dim = Pos()) : pos(pos), dim(dim) {}
    sf::Rect<float> boundingBox(Transform tf);
    bool mouseTouching(const MouseState& mouse, Transform tf);
};

struct TextBox : public MouseRegion {
    float font_size;
    std::string value;
    static sf::Font font;
    static void init(sf::Font font);
    bool align_middle; // centered on vertical axis
    TextBox(Pos pos = Pos(), Pos dim = Pos(), float font_size = 1.f, std::string init_value = "", bool align_middle = false) : MouseRegion(pos, dim), font_size(font_size), value(init_value), align_middle(align_middle) {}
    void draw(sf::RenderWindow& window, Transform tf);
};

struct TaskEditor {
    TextBox tb_phase;
    TextBox tb_period;
    TextBox tb_exec;
    TextBox tb_deadline;

    TaskEditor() {}

    void draw(sf::RenderWindow& window, Transform tf, Task& task, int tid);
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
    Pos getPos(bool task_based) const;
    Pos getDim() const;
    bool mouseTouching(const MouseState& mouse, Transform tf, Transform core_tracks_offset); // return true if mouse hovered over
    void draw(sf::RenderWindow& window, Transform tf, bool task_based, bool focused) const;
};

struct Visualizer {
    static sf::Font font;
    static void init();
    
    sf::RenderWindow window;
    Transform tf = Transform::scale(START_ZOOM, START_ZOOM);
    std::vector<std::vector<ExecBlockView>> blocks;

    std::vector<TaskEditor> task_editors;
    std::vector<TextBox> task_labels;
    std::vector<TextBox> core_labels;

    Visualizer() {}

    // opens window
    void open(unsigned int width, unsigned int height);

    // closes window
    void close();

    // returns true if window open false otherwise
    bool isOpen();

    // handle zoom
    void zoom(float sx, float sy, Pos zoom_origin);

    // handle movement
    void move(float dx, float dy);

    // updates display
    void update(SimModel& model, const MouseState& mouse, float fps);
};

#endif