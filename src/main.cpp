#include "model.h"
#include "view.h"
#include "schedulers.h"
#include <iostream>
#include <cmath>
#include <deque>
#include <algorithm>

int main() {
    Visualizer::init();
    SimModel model;
    // setup test model
    TaskSet tset;
    tset.push_back(Task(20, 15));
    tset.push_back(Task(10, 5));
    tset.push_back(Task(20, 8));
    tset.push_back(Task(10, 8));
    tset.push_back(Task(20, 11));
    // tset.push_back(Task(Fraction(11,12), Fraction(2,9)));
    Scheduler* scheduler = new EDZL();
    scheduler->init(tset);
    model.reset(tset, scheduler, 3);

    Visualizer view;
    view.tf = Transform::scale(START_ZOOM) * Transform::id();
    const float init_scale = 0.8f;
    view.open(sf::VideoMode::getDesktopMode().width * init_scale, sf::VideoMode::getDesktopMode().height * init_scale);
    MouseState mouse;
    mouse.mouse_down = false;
    mouse.mouse_lost = true;
    sf::Clock clock;
    std::deque<long long> frame_time_history;
    long long frame_time_sum = 0.f;
    bool left_pressed = false;
    bool right_pressed = false;
    bool down_pressed = false;
    while (view.window.isOpen()) {
        clock.restart();
        float fps = 1000000.f * (float)frame_time_history.size() / (float)frame_time_sum;

        // input step - handle events
        Pos new_mouse_pos = mouse.pos;
        bool mouse_moved = false;
        bool old_mouse_down = mouse.mouse_down;
        int zoom_action = 0;
        KeyState::stepTick();
        for (auto event = sf::Event{}; view.window.pollEvent(event);) {
            switch (event.type) {
                case sf::Event::Closed:
                    view.close();
                    break;
                case sf::Event::MouseWheelMoved:
                    if (event.mouseWheel.delta > 0)
                        zoom_action = 1;
                    else if (event.mouseWheel.delta < 0)
                        zoom_action = -1;
                    break;
                case sf::Event::LostFocus:
                case sf::Event::MouseLeft:
                    mouse.mouse_down = false;
                case sf::Event::GainedFocus:
                    mouse.mouse_lost = true;
                    left_pressed = false;
                    right_pressed = false;
                    KeyState::reset();
                    break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        mouse.mouse_down = true;
                        mouse.mouse_lost = true;
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) mouse.mouse_down = false;
                    break;
                case sf::Event::MouseMoved:
                    mouse_moved = true;
                    new_mouse_pos.x = event.mouseMove.x;
                    new_mouse_pos.y = event.mouseMove.y;
                    break;
                case sf::Event::KeyPressed:
                    KeyState::pressKey(event.key.code);
                    break;
                case sf::Event::KeyReleased:
                    KeyState::releaseKey(event.key.code);
                    break;
            }
        }

        // handle mouse actions
        if (mouse_moved) {
            if (mouse.mouse_lost) {
                mouse.mouse_lost = false;
                mouse.pos = new_mouse_pos;
            }
            Pos mouse_pos_change = new_mouse_pos - mouse.pos;
            if (mouse.mouse_down) {
                view.move(-mouse_pos_change.x, -mouse_pos_change.y);
            }
            mouse.pos = new_mouse_pos;
        }
        mouse.just_changed = mouse.mouse_down != old_mouse_down;
        const float zf_in = 1.02f;
        const float zf_out = 1.f / zf_in;
        const float move_amount = 1.f;
        if (zoom_action == 1)
            view.zoom(1.25f, 1.25f, mouse.pos);
        if (zoom_action == -1)
            view.zoom(0.8f, 0.8f, mouse.pos);
        if (KeyState::keyPressed(sf::Keyboard::Right))
            view.zoom(zf_in, 1.f, Pos(view.window.getSize()) * 0.5f);
        if (KeyState::keyPressed(sf::Keyboard::Left))
            view.zoom(zf_out, 1.f, Pos(view.window.getSize()) * 0.5f);
        if (KeyState::keyPressed(sf::Keyboard::Up) || KeyState::keyPressed(sf::Keyboard::Down)) {
            Transform ttf = Transform::scale(view.tf.sy(), view.tf.sy()) * view.tf.scale().inv();
            ttf.sx() = std::clamp(ttf.sx(), 0.8f, 1.25f);
            view.zoom(ttf.sx(), ttf.sy(), Pos(view.window.getSize()) * 0.5f);
        }
        bool stretched = std::abs(view.tf.sx() / view.tf.sy() - 1.f) > 0.00001f;
        if (KeyState::keyPressed(sf::Keyboard::Up) && !stretched)
            view.zoom(zf_in, zf_in, Pos(view.window.getSize()) * 0.5f);
        if (KeyState::keyPressed(sf::Keyboard::Down) && !stretched)
            view.zoom(zf_out, zf_out, Pos(view.window.getSize()) * 0.5f);
        if (KeyState::keyPressed(sf::Keyboard::A))
            view.move(-move_amount, 0);
        if (KeyState::keyPressed(sf::Keyboard::D))
            view.move(move_amount, 0);
        if (KeyState::keyPressed(sf::Keyboard::W))
            view.move(0, -move_amount);
        if (KeyState::keyPressed(sf::Keyboard::S))
            view.move(0, move_amount);

        // calc step - update model
        int end_time = (int)std::ceil((view.tf.inv() * Pos(view.window.getSize())).x);
        model.sim(end_time);

        // draw step - update view
        view.update(model, mouse, fps);

        // timing analysis
        long long frame_time = clock.getElapsedTime().asMicroseconds();
        frame_time_history.push_back(frame_time);
        frame_time_sum += frame_time;
        if (frame_time_history.size() > 60) {
            frame_time_sum -= frame_time_history.front();
            frame_time_history.pop_front();
        }
    }
}