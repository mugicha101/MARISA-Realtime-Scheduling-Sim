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

void View::update(Model& model, const MouseState& mouse) {
    TaskSim& sim = model.sim;
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, window.getSize().x, window.getSize().y)));
    window.clear(sf::Color(200, 200, 200));

    // handle new exec blocks
    blocks.resize(sim.task_set.size());
    while (sim.ebs.hasNext()) {
        ExecBlock block = sim.ebs.getNext();
        std::vector<ExecBlockView>& task_blocks = blocks[block.task_id];
        if (!task_blocks.empty()) {
            const ExecBlock& backBlock = task_blocks.back().block;
            if (backBlock.end == block.start && backBlock.job_id == block.job_id && backBlock.core == block.core) {
                block.start = backBlock.start;
                task_blocks.pop_back();
            }
        }
        blocks[block.task_id].emplace_back(block);
    }

    // helper function to find first and last block in range using bsearch
    auto taskViewBounds = [&](std::vector<ExecBlockView>& task_blocks, float left, float right) {
        int start, end;
        int l = 0;
        int r = task_blocks.size()-1;
        while (l != r) {
            int m = (l + r) >> 1;
            if ((tf * task_blocks[m].getPos(false, block_stretch)).x + (task_blocks[m].getDim(block_stretch)).x * tf.sx() >= left)
                r = m;
            else
                l = m + 1;
        }
        start = l;
        l = 0;
        r = task_blocks.size()-1;
        while (l != r) {
            int m = (l + r + 1) >> 1;
            if ((tf * task_blocks[m].getPos(false, block_stretch)).x <= right)
                l = m;
            else
                r = m - 1;
        }
        end = l;
        return std::make_pair(start, end);
    };

    // figure out which blocks are being hovered on
    Transform core_tracks_offset = Transform::trans(0.f, BLOCK_SPACING * (sim.task_set.size() + 1));
    int hover_tid = -1, hover_jid = -1;
    for (int tid = 0; hover_tid == -1 && tid < sim.task_set.size(); ++tid) {
        std::vector<ExecBlockView>& task_blocks = blocks[tid];
        if (task_blocks.empty()) continue;
        std::pair<int,int> bounds = taskViewBounds(task_blocks, mouse.pos.x, mouse.pos.x);
        if (bounds.first != bounds.second) continue;
        ExecBlockView& block_view = task_blocks[bounds.first];
        if (!block_view.mouseTouching(mouse, tf, core_tracks_offset, block_stretch)) continue;
        hover_tid = tid;
        hover_jid = block_view.block.job_id;
    }

    // render each tasks exec blocks
    std::vector<ExecBlockView*> focused_block_views;
    Transform tasks_tf = tf;
    Transform cores_tf = tf * core_tracks_offset;
    for (int tid = 0; tid < sim.task_set.size(); ++tid) {
        std::vector<ExecBlockView>& task_blocks = blocks[tid];
        if (task_blocks.empty()) continue;
        std::pair<int,int> bounds = taskViewBounds(task_blocks, 0, window.getSize().x);
        int start = bounds.first;
        int end = bounds.second;
        for (int i = start; i <= end; ++i) {
            ExecBlockView& block_view = task_blocks[i];
            if (hover_tid != -1 && tid == hover_tid && block_view.block.job_id == hover_jid) {
                focused_block_views.push_back(&block_view);
                continue;
            }
            block_view.draw(window, tasks_tf, block_stretch, true, hover_tid == -1);
            block_view.draw(window, cores_tf, block_stretch, false, hover_tid == -1);
        }
    }

    for (ExecBlockView* block_view : focused_block_views) {
        block_view->draw(window, tasks_tf, block_stretch, true, true);
        block_view->draw(window, cores_tf, block_stretch, false, true);
    }

    // draw ui


    // display call
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

sf::Color hue(float v, float lighten = 0.f, float scale = 255.f) {
    auto calc = [lighten, scale](float f) {
        f = f - floor(f);
        f = std::clamp(2.f - (f > 0.5f ? 1.f - f : f) * 6.f, 0.f, 1.f);
        return (f + (1.f - f) * lighten) * scale;
    };
    return sf::Color(
        calc(v),
        calc(v + (2.f / 3.f)),
        calc(v + (1.f / 3.f))
    );
};

bool ExecBlockView::mouseTouching(const MouseState& mouse, Transform tf, Transform core_tracks_offset, int block_stretch) {
    task_mr.pos = getPos(true, block_stretch);
    core_mr.pos = getPos(false, block_stretch);
    task_mr.dim = core_mr.dim = getDim(block_stretch);
    return task_mr.mouseTouching(mouse, tf) || core_mr.mouseTouching(mouse, tf * core_tracks_offset);
}

void ExecBlockView::draw(sf::RenderWindow& window, Transform tf, int block_stretch, bool task_based, bool focused) const {
    // draw block
    int fc = focused ? 255 : 128;
    sf::RectangleShape rect(*(tf.scale() * getDim(block_stretch)));
    Pos blockPos = tf * getPos(task_based, block_stretch);
    rect.move(*blockPos);
    sf::Color color = hue((task_based ? block.job_id : block.task_id) / 12.f, 0.25f, (float)fc);
    if (std::min(rect.getSize().x, rect.getSize().y) <= BLOCK_OUTLINE * 2.f) {
        rect.setFillColor(color);
        window.draw(rect);
        return;
    }
    sf::Rect base_rect = rect.getGlobalBounds();
    rect.move(BLOCK_OUTLINE, BLOCK_OUTLINE);
    rect.setSize(sf::Vector2f(rect.getSize().x - BLOCK_OUTLINE * 2.f, rect.getSize().y - BLOCK_OUTLINE * 2.f));
    rect.setFillColor(sf::Color(fc, fc, fc));
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
            float bar_height = (BLOCK_SPACING - BLOCK_HEIGHT) * 0.5f * tf.sy();
            finRect.setPosition(base_rect.left + base_rect.width - BLOCK_OUTLINE * 0.5f, base_rect.top - bar_height * 0.5f);
            finRect.setFillColor(color);
            finRectDraw(BLOCK_OUTLINE, bar_height);
            finRect.setPosition(base_rect.left + base_rect.width - BLOCK_OUTLINE * 0.5f, base_rect.top - bar_height);
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

bool MouseRegion::mouseTouching(const MouseState& mouse, Transform tf) {
    sf::Rect rect(*(tf * pos), *(tf.scale() * dim));
    return rect.contains(*mouse.pos);
}