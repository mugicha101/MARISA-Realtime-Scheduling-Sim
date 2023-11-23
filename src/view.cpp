#include "view.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <filesystem>

const float BLOCK_SPACING = 10.f;
const float BLOCK_HEIGHT = 5.f;
const float TEXT_MARGIN = 0.1f;
const float BLOCK_OUTLINE = 3.f;

void View::init() {
    ExecBlockView::init();
}

void View::open(unsigned int width, unsigned int height) {
    window.create(sf::VideoMode(width, height), "Multicore And Realtime Interactive Scheduling Analyzer (MARISA)");
}

void View::close() {
    window.close();
}

bool View::isOpen() {
    return window.isOpen();
}

void View::update(Model& model) {
    window.clear(sf::Color(200, 200, 200));
    blocks.resize(model.sim.task_set.size());

    // handle new exec blocks
    while (model.sim.ebs.hasNext()) {
        ExecBlock block = model.sim.ebs.getNext();
        std::vector<ExecBlockView>& taskBlocks = blocks[block.task_id];
        if (!taskBlocks.empty()) {
            const ExecBlock& backBlock = taskBlocks.back().block;
            if (backBlock.end == block.start && backBlock.job_id == block.job_id && backBlock.core == block.core) {
                block.start = backBlock.start;
                taskBlocks.pop_back();
            }
        }
        blocks[block.task_id].emplace_back(block);
    }

    // render each tasks exec blocks
    Transform core_tracks_offset = Transform::trans(0.f, BLOCK_SPACING * (model.sim.task_set.size() + 1));
    for (int tid = 0; tid < model.sim.task_set.size(); ++tid) {
        std::vector<ExecBlockView>& taskBlocks = blocks[tid];
        Transform ltf = tf;
        // find first and last block in range using bsearch
        int start, end;
        {
            int l = 0;
            int r = taskBlocks.size()-1;
            while (l != r) {
                int m = (l + r) >> 1;
                if ((tf * taskBlocks[m].getPos(false, block_stretch)).x + (taskBlocks[m].getDim(block_stretch)).x * tf.sx() >= 0)
                    r = m;
                else
                    l = m + 1;
            }
            start = l;
            l = 0;
            r = taskBlocks.size()-1;
            while (l != r) {
                int m = (l + r + 1) >> 1;
                if ((tf * taskBlocks[m].getPos(false, block_stretch)).x <= window.getSize().x)
                    l = m;
                else
                    r = m - 1;
            }
            end = l;
        }

        // render task-based view
        for (int i = start; i <= end; ++i) {
            taskBlocks[i].draw(window, ltf, block_stretch, true);
        }

        // render core-based view
        ltf = ltf * core_tracks_offset;
        for (int i = start; i <= end; ++i) {
            taskBlocks[i].draw(window, ltf, block_stretch, false);
        }
    }

    // DEBUG
    auto debug = [&](float x, float y) {
        Pos pos = tf * Pos(x, y);
        Pos dim = tf.scale() * Pos(1, 1);
        sf::RectangleShape rect(*dim);
        rect.setPosition(*pos);
        rect.setOrigin(rect.getSize().x * 0.5f, rect.getSize().y * 0.5f);
        rect.setFillColor(sf::Color::Red);
        window.draw(rect);
    };
    for (int x = -10; x <= 10; ++x) {
        for (int y = -10; y <= 10; ++y) {
            debug(x * 10, y * 10);
        }
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

Pos ExecBlockView::getPos(bool task_based, int block_stretch) const {
    return {getX() * block_stretch, getY(task_based)};
}

Pos ExecBlockView::getDim(int block_stretch) const {
    return {getWidth() * block_stretch, getHeight()};
}

sf::Color hue(float v, float lighten) {
    auto calc = [lighten](float f) {
        f = f - floor(f);
        f = std::clamp(2.f - (f > 0.5f ? 1.f - f : f) * 6.f, 0.f, 1.f);
        return (f + (1.f - f) * lighten) * 255.f;
    };
    return sf::Color(
        calc(v),
        calc(v + (2.f / 3.f)),
        calc(v + (1.f / 3.f))
    );
};

void ExecBlockView::draw(sf::RenderWindow& window, Transform tf, int block_stretch, bool task_based) const {
    // draw block
    sf::RectangleShape rect(*(tf.scale() * getDim(block_stretch)));
    Pos blockPos = tf * getPos(task_based, block_stretch);
    rect.move(*blockPos);
    sf::Color color = hue((task_based ? block.job_id : block.task_id) / 12.f, 0.25f);
    if (std::min(rect.getSize().x, rect.getSize().y) <= BLOCK_OUTLINE * 2.f) {
        rect.setFillColor(color);
        window.draw(rect);
        return;
    }
    sf::Rect baseRect = rect.getGlobalBounds();
    rect.move(BLOCK_OUTLINE, BLOCK_OUTLINE);
    rect.setSize(sf::Vector2f(rect.getSize().x - BLOCK_OUTLINE * 2.f, rect.getSize().y - BLOCK_OUTLINE * 2.f));
    rect.setFillColor(sf::Color::White);
    rect.setOutlineColor(color);
    rect.setOutlineThickness(BLOCK_OUTLINE);
    window.draw(rect);

    // draw text
    if (rect.getSize().x > 10.f) {
        blockPos = Pos(rect.getPosition()) + Pos(rect.getSize()) * 0.5f;
        sf::Text text;
        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(200);
        text.setFillColor(sf::Color::Black);
        text.setPosition(blockPos.x, blockPos.y);
        text.setStyle(sf::Text::Bold);
        sf::Rect textBounds = text.getGlobalBounds();
        text.setOrigin(*(Pos(textBounds.getPosition()) - Pos(text.getPosition()) + Pos(textBounds.getSize()) * 0.5f));
        float scale = std::min(0.15f, std::min(rect.getSize().x / (textBounds.width * (1.f + TEXT_MARGIN)), rect.getSize().y / (textBounds.height  * (1.f + TEXT_MARGIN))));
        text.setScale(scale, scale);
        window.draw(text);
    }

    // draw marks based on end state
    switch (block.endState) {
        case ExecBlock::PREEMPTED:
            break;
        case ExecBlock::COMPLETED: {
            sf::RectangleShape finRect;
            auto finRectDraw = [&](float width, float height) {
                finRect.setOrigin(width * 0.5f, height * 0.5f);
                finRect.setSize(sf::Vector2f(width, height));
                window.draw(finRect);
            };
            float barHeight = (BLOCK_SPACING - BLOCK_HEIGHT) * 0.5f * tf.sy();
            finRect.setPosition(baseRect.left + baseRect.width - BLOCK_OUTLINE * 0.5f, baseRect.top - barHeight * 0.5f);
            finRect.setFillColor(color);
            finRectDraw(BLOCK_OUTLINE, barHeight);
            finRect.setPosition(baseRect.left + baseRect.width - BLOCK_OUTLINE * 0.5f, baseRect.top - barHeight);
            finRectDraw(std::min(20.f, 2.f * tf.sx() * block_stretch), BLOCK_OUTLINE);
        } break;
        case ExecBlock::MISSED: {

        } break;
    }
}

sf::Font ExecBlockView::font;

void ExecBlockView::init() {
    if (!font.loadFromFile("resources/font.otf")) {
        std::cout << "failed to load font" << std::endl;
    }
}