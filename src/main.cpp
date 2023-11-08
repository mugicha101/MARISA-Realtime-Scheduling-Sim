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
    Scheduler* scheduler = new GEDF(true);
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
    return 0;

    View view;
    const float initScale = 0.8f;
    view.open(sf::VideoMode::getDesktopMode().width * initScale, sf::VideoMode::getDesktopMode().height * initScale);
    while (view.window.isOpen()) {
        for (auto event = sf::Event{}; view.window.pollEvent(event);) {
            if (event.type == sf::Event::Closed)
                view.close();
        }
        view.update(model);
    }
}