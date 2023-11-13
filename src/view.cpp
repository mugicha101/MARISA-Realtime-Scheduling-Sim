#include "view.h"
#include <vector>
#include <cmath>

const int BLOCK_SPACING = 10;
const int BLOCK_HEIGHT = 5;

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
    Transform ltf = tf;

    // render task-based view
    for (ExecBlock& block : blocks) {
        ExecBlockView view(block);
        view.draw(window, ltf, true);
    }

    // render core-based view
    ltf.dy += BLOCK_SPACING * (model.sim.task_set.size() + 1);
    for (ExecBlock& block : blocks) {
        ExecBlockView view(block);
        view.draw(window, ltf, false);
    }
    window.display();
}

float ExecBlockView::getX() const {
    return (float)(block.start);
}

float ExecBlockView::getY(bool task_based) const {
    return (float)((task_based ? block.task_id : block.core) * BLOCK_SPACING + (BLOCK_SPACING - BLOCK_HEIGHT));
}

float ExecBlockView::getWidth() const {
    return (float)(block.end - block.start);
}

float ExecBlockView::getHeight() const {
    return (float)BLOCK_HEIGHT;
}

Pos ExecBlockView::getPos(bool task_based) const {
    return {getX(), getY(task_based)};
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

void ExecBlockView::draw(sf::RenderWindow& window, Transform tf, bool task_based) const {
    sf::RectangleShape rect(*(getDim() * tf.scale));
    rect.move(*(getPos(task_based) * tf));
    rect.setFillColor(hue((task_based ? block.job_id : block.task_id) / 12.f));
    window.draw(rect);
}