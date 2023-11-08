#include "view.h"

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
    window.display();
}