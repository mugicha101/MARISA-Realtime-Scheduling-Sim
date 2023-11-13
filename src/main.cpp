#include "model.h"
#include "view.h"
#include "schedulers.h"
#include <iostream>

int main() {
    Model model;

    // setup test model
    TaskSet tset;
    tset.push_back(Task(10, 5));
    tset.push_back(Task(3, 2));
    tset.push_back(Task(14, 2));
    Scheduler* scheduler = new GFIFO();
    scheduler->init(tset);
    model.sim.reset(tset, scheduler, 2);
    printf("SEEKING TO 100\n");
    model.sim.seek(100);
    printf("END (MISSED JOB: %d)\n", model.sim.missed);
    printf("CORE VISUALIZATION:\n");
    std::vector<ExecBlock> blocks = model.sim.ebs.dump();
    std::sort(blocks.begin(), blocks.end(), [](ExecBlock& a, ExecBlock& b) {
        return a.start < b.start;
    });
    std::vector<std::pair<int,int>> end_heap;
    std::vector<std::vector<ExecBlock>> core_blocks(model.sim.cores);
    for (ExecBlock& block : blocks)
        core_blocks[block.core].push_back(block);
    for (int c = 0; c < model.sim.cores; ++c) {
        std::cout << c+1 << ": ";
        int t = 0;
        for (ExecBlock& block : core_blocks[c]) {
            while (t < block.start) {
                std::cout << " ";
                ++t;
            }
            while (t < block.end) {
                std::cout << block.task_id+1;
                ++t;
            }
        }
        std::cout << std::endl;
    }

    View view;
    const float initScale = 0.8f;
    view.open(sf::VideoMode::getDesktopMode().width * initScale, sf::VideoMode::getDesktopMode().height * initScale);
    bool mouse_down = false;
    Pos mpos;
    bool mouse_lost = true;
    while (view.window.isOpen()) {
        Pos nmpos = mpos;
        bool mouse_moved = false;
        int zoom_action = 0.f;
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
                case sf::Event::MouseLeft:
                    mouse_down = false;
                    break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) mouse_down = true;
                    break;
                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) mouse_down = false;
                    mouse_lost = true;
                    break;
                case sf::Event::MouseMoved:
                    mouse_moved = true;
                    nmpos.x = event.mouseMove.x;
                    nmpos.y = event.mouseMove.y;
                    break;
            }
        }
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
            Pos offset(view.tf.dx, view.tf.dy);
            offset = offset + mpos / (view.tf.scale * zoom_factor) - mpos / view.tf.scale;
            view.tf.dx = offset.x;
            view.tf.dy = offset.y;
            view.tf.scale *= zoom_factor;
        }
        view.update(model);
    }
}