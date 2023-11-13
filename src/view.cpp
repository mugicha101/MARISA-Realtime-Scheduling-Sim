#include "view.h"
#include <vector>
#include <cmath>

void View::open(unsigned int width, unsigned int height) {
    window.create(sf::VideoMode(width, height), "Marisa");
}

void View::close() {
    window.close();
}

bool View::isOpen() {
    return window.isOpen();
}

void View::update(Model& model) {
    window.clear(sf::Color(200, 200, 200));
    std::vector<ExecBlock> blocks = model.sim.ebs.dump();
    for (ExecBlock& block : blocks) {
        ExecBlockView view(block);
        view.draw(window, tf);
    }
    window.display();
}

const int BLOCK_SPACING = 10;
const int BLOCK_HEIGHT = 5;

float ExecBlockView::getX() const {
    return (float)(block.start);
}

float ExecBlockView::getY() const {
    return (float)(block.task_id * BLOCK_SPACING + (BLOCK_SPACING - BLOCK_HEIGHT));
}

float ExecBlockView::getWidth() const {
    return (float)(block.end - block.start);
}

float ExecBlockView::getHeight() const {
    return (float)BLOCK_HEIGHT;
}

Pos ExecBlockView::getPos() const {
    return {getX(), getY()};
}

Pos ExecBlockView::getDim() const {
    return {getWidth(), getHeight()};
}

sf::Color hue(float v) {
    auto calc = [](float f) {
        f = f - floor(f);
        return std::clamp(2.f - (f > 0.5f ? 1.f - f : f) * 6.f, 0.f, 1.f) * 255.f;
    };
    return sf::Color(
        calc(v),
        calc(v + (2.f / 3.f)),
        calc(v + (1.f / 3.f))
    );
};

void ExecBlockView::draw(sf::RenderWindow& window, Transform tf) const {
    sf::RectangleShape rect(*(getDim() * tf.scale));
    rect.move(*(getPos() * tf));
    rect.setFillColor(hue(block.job_id * 1.f / 12.f));
    window.draw(rect);
}