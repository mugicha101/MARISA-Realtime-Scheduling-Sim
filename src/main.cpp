#include "model.h"
#include "view.h"
#include "schedulers.h"
#include <iostream>
#include <cmath>

const float MAX_ZOOM = 20.f;
const float MIN_ZOOM = 0.5f;

int main() {
    Model model;
    // setup test model
    TaskSet tset;
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(3, 1));
    tset.push_back(Task(9, 4));
    tset.push_back(Task(9, 4));
    tset.push_back(Task(9, 4));
    Scheduler* scheduler = new PD2(4);
    scheduler->init(tset);
    model.sim.reset(tset, scheduler, 4);

    View view;
    const float init_scale = 0.8f;
    view.open(sf::VideoMode::getDesktopMode().width * init_scale, sf::VideoMode::getDesktopMode().height * init_scale);
    bool mouse_down = false;
    Pos mpos;
    bool mouse_lost = true;
    while (view.window.isOpen()) {
        Pos nmpos = mpos;
        bool mouse_moved = false;
        int zoom_action = 0.f;

        // input step - handle events
        for (auto event = sf::Event{}; view.window.pollEvent(event);) {
            switch (event.type) {
                case sf::Event::Closed:
                    view.close();
                    break;
                case sf::Event::MouseWheelMoved:
                    if (event.mouseWheel.delta > 0) {
                        zoom_action = 1;
                    } else if (event.mouseWheel.delta < 0) {
                        zoom_action = -1;
                    }
                    break;
                case sf::Event::LostFocus:
                case sf::Event::MouseLeft:
                    mouse_down = false;
                case sf::Event::GainedFocus:
                    mouse_lost = true;
                    break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        mouse_down = true;
                        mouse_lost = true;
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) mouse_down = false;
                    break;
                case sf::Event::MouseMoved:
                    mouse_moved = true;
                    nmpos.x = event.mouseMove.x;
                    nmpos.y = event.mouseMove.y;
                    break;
            }
        }

        // handle mouse actions
        if (mouse_moved) {
            if (mouse_lost) {
                mouse_lost = false;
                mpos = nmpos;
            }
            Pos dmpos = nmpos - mpos;
            if (mouse_down) {
                view.tf.dx += dmpos.x / view.tf.scale;
                view.tf.dy += dmpos.y / view.tf.scale;
            }
            mpos = nmpos;
        }
        if (zoom_action) {
            float zoom_factor = zoom_action == 1 ? 1.25f : 0.8f;
            if (view.tf.scale * zoom_factor <= MAX_ZOOM && view.tf.scale * zoom_factor >= MIN_ZOOM) {
                Pos offset(view.tf.dx, view.tf.dy);
                offset = offset + mpos / (view.tf.scale * zoom_factor) - mpos / view.tf.scale;
                view.tf.dx = offset.x;
                view.tf.dy = offset.y;
                view.tf.scale *= zoom_factor;
            }
        }

        // calc step - update model
        int end_time = (int)std::ceil((Pos(view.window.getSize()) / view.tf).x);
        model.sim.sim(end_time);

        // draw step - update view
        view.update(model);
    }
}