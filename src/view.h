#ifndef VIEW_H
#define VIEW_H

#include <SFML/Graphics.hpp>
#include "model.h"

struct View {
    sf::RenderWindow window;

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