#include "view.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <unordered_map>

const float BLOCK_SPACING = 10.f;
const float BLOCK_HEIGHT = 5.f;
const float TEXT_MARGIN = 0.1f;
const float BLOCK_OUTLINE = 3.f;

sf::Font Visualizer::font;
void Visualizer::init() {
    if (!font.loadFromFile("resources/font.otf")) {
        std::cout << "failed to load font" << std::endl;
    }
    ExecBlockView::init(font);
    TextBox::init(font);
}

void Visualizer::open(unsigned int width, unsigned int height) {
    window.create(sf::VideoMode(width, height), "Multicore And Realtime Interactive Scheduling Analyzer (MARISA)");
}

void Visualizer::close() {
    window.close();
}

bool Visualizer::isOpen() {
    return window.isOpen();
}

void Visualizer::update(SimModel& model, const MouseState& mouse, float fps) {
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, window.getSize().x, window.getSize().y)));

    // draw background grid
    window.clear(sf::Color(200, 200, 200));
    float offset = tf.sx();
    while (offset < 20.f) offset *= 10.f;
    float grid_pos = (tf * Pos(0.f, 0.f)).x - BLOCK_OUTLINE * 0.5f;
    grid_pos = std::max(grid_pos, std::fmod(std::fmod(grid_pos, offset) + offset, offset));
    sf::RectangleShape rect(sf::Vector2f(BLOCK_OUTLINE, window.getSize().y));
    rect.setFillColor(sf::Color(180, 180, 180));
    while (grid_pos < window.getSize().x) {
        rect.setPosition(grid_pos, 0.f);
        window.draw(rect);
        grid_pos += offset;
    }

    // handle new exec blocks
    blocks.resize(model.task_set.size());
    while (model.ebs.hasNext()) {
        ExecBlock block = model.ebs.getNext();
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
            if ((tf * task_blocks[m].getPos(false)).x + (task_blocks[m].getDim()).x * tf.sx() >= left)
                r = m;
            else
                l = m + 1;
        }
        start = l;
        l = 0;
        r = task_blocks.size()-1;
        while (l != r) {
            int m = (l + r + 1) >> 1;
            if ((tf * task_blocks[m].getPos(false)).x <= right)
                l = m;
            else
                r = m - 1;
        }
        end = l;
        return std::make_pair(start, end);
    };

    // figure out which blocks are being hovered on
    Transform core_tracks_offset = Transform::trans(0.f, BLOCK_SPACING * (model.task_set.size() + 1));
    int hover_tid = -1, hover_jid = -1;
    for (int tid = 0; hover_tid == -1 && tid < model.task_set.size(); ++tid) {
        std::vector<ExecBlockView>& task_blocks = blocks[tid];
        if (task_blocks.empty()) continue;
        std::pair<int,int> bounds = taskViewBounds(task_blocks, mouse.pos.x, mouse.pos.x);
        if (bounds.first != bounds.second) continue;
        ExecBlockView& block_view = task_blocks[bounds.first];
        if (!block_view.mouseTouching(mouse, tf, core_tracks_offset)) continue;
        hover_tid = tid;
        hover_jid = block_view.block.job_id;
    }

    // render each tasks exec blocks
    std::vector<ExecBlockView*> focused_block_views;
    Transform tasks_tf = tf;
    Transform cores_tf = tf * core_tracks_offset;
    for (int tid = 0; tid < model.task_set.size(); ++tid) {
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
            block_view.draw(window, tasks_tf, true, hover_tid == -1);
            block_view.draw(window, cores_tf, false, hover_tid == -1);
        }
    }
    for (ExecBlockView* block_view : focused_block_views) {
        block_view->draw(window, tasks_tf, true, true);
        block_view->draw(window, cores_tf, false, true);
    }

    // draw ui
    task_editors.resize(model.task_set.size());
    for (int tid = 0; tid < model.task_set.size(); ++tid) {
        task_editors[tid].draw(window, tf, model.task_set[tid], tid);
    }

    // draw debug
    sf::Text text;
    text.setFillColor(sf::Color::Blue);
    text.setFont(font);
    text.setCharacterSize(15);
    text.setString(std::to_string((int)std::floor(fps)) + " fps");
    sf::Rect text_bounds = text.getGlobalBounds();
    text.setPosition(window.getSize().x - text_bounds.getSize().x - 5.f, 5.f);
    window.draw(text);

    // display call
    window.display();
}

void Visualizer::zoom(float sx, float sy, Pos zoom_origin) {
    Transform stf = Transform::scale(sx, sy);
    Transform ntf = stf * tf;
    if (std::max(ntf.sx(), ntf.sy()) > MAX_ZOOM || std::min(ntf.sx(), ntf.sy()) < MIN_ZOOM)
        return;
    tf = Transform::trans(zoom_origin.x, zoom_origin.y) * stf * Transform::trans(-zoom_origin.x, -zoom_origin.y) * tf;
}

void Visualizer::move(float dx, float dy) {
    tf = Transform::trans(-dx, -dy) * tf;
}

float ExecBlockView::getX() const {
    return *block.start;
}

float ExecBlockView::getY(bool task_based) const {
    return (float)((task_based ? block.task_id : block.core) * BLOCK_SPACING + (BLOCK_SPACING - BLOCK_HEIGHT));
}

float ExecBlockView::getWidth() const {
    return (float)(*block.end - *block.start);
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

bool ExecBlockView::mouseTouching(const MouseState& mouse, Transform tf, Transform core_tracks_offset) {
    task_mr.pos = getPos(true);
    core_mr.pos = getPos(false);
    task_mr.dim = core_mr.dim = getDim();
    return task_mr.mouseTouching(mouse, tf) || core_mr.mouseTouching(mouse, tf * core_tracks_offset);
}

void ExecBlockView::draw(sf::RenderWindow& window, Transform tf, bool task_based, bool focused) const {
    // draw block
    int fc = focused ? 255 : 128;
    sf::RectangleShape rect(*(tf.scale() * getDim()));
    Pos blockPos = tf * getPos(task_based);
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
            finRectDraw(std::min(20.f, 2.f * tf.sx()), BLOCK_OUTLINE);
        } break;
        case ExecBlock::MISSED: {

        } break;
    }
}

sf::Font ExecBlockView::font;

void ExecBlockView::init(sf::Font font) {
    ExecBlockView::font = font;
}

sf::Font TextBox::font;

void TextBox::init(sf::Font font) {
    TextBox::font = font;
}

void TextBox::draw(sf::RenderWindow& window, Transform tf) {
    sf::Rect<float> bounds = boundingBox(tf);
    sf::RectangleShape rect(bounds.getSize());
    rect.setPosition(bounds.getPosition());
    rect.setFillColor(sf::Color::White);
    rect.setOutlineColor(sf::Color::Black);
    rect.setOutlineThickness(BLOCK_OUTLINE);
    window.draw(rect);
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(200);
    text.setScale(tf.sx() * 0.01 * font_size, tf.sy() * 0.01 * font_size);
    text.setFillColor(sf::Color::Black);
    text.setStyle(sf::Text::Bold);
    text.setPosition(rect.getPosition());
    text.setLineSpacing(0.6f);
    std::string line = "";
    for (char c : value) {
        line += c;
        text.setString(line);
        if (text.getGlobalBounds().getSize().x > rect.getSize().x && line.size() > 1) {
            line.pop_back();
            line += '\n';
            line += c;
        }
    }
    text.setString(line);
    if (align_middle) {
        text.setOrigin(text.getLocalBounds().getPosition() + sf::Vector2f(0.f, text.getLocalBounds().getSize().y * 0.5f));
        text.move(sf::Vector2f(0.f, rect.getSize().y * 0.5f));
    } else {
        text.setOrigin(text.getLocalBounds().getPosition());
    }
    window.draw(text);
    line = "";
    text.setPosition(sf::Vector2f(text.getPosition().x, text.getPosition().y + text.getGlobalBounds().height));
}

void TaskEditor::draw(sf::RenderWindow& window, Transform tf, Task& task, int tid) {
    std::vector<TextBox*> tb_arr = {&tb_phase, &tb_period, &tb_exec, &tb_deadline};
    tb_phase = TextBox(Pos(0, tid * BLOCK_SPACING), Pos(10, 5), 3.f, "test", true);
    tb_phase.draw(window, tf);
    tb_period = TextBox(Pos(0, tid * BLOCK_SPACING), Pos(10, 5), 3.f, "test", true);
}

sf::Rect<float> MouseRegion::boundingBox(Transform tf) {
    return sf::Rect<float>(*(tf * pos), *(tf.scale() * dim));
}

bool MouseRegion::mouseTouching(const MouseState& mouse, Transform tf) {
    return boundingBox(tf).contains(*mouse.pos);
}

int KeyState::tick = 0;
std::unordered_map<sf::Keyboard::Key, KeyState> KeyState::key_map;

void KeyState::stepTick() {
    ++tick;
}

void KeyState::pressKey(sf::Keyboard::Key key) {
    KeyState& state = key_map[key];
    state.type_tick = tick;
    if (state.key_down) return;
    state.key_down = true;
    state.update_tick = tick;
}

void KeyState::releaseKey(sf::Keyboard::Key key) {
    KeyState& state = key_map[key];
    if (!state.key_down) return;
    state.key_down = false;
    state.update_tick = tick;
}

void KeyState::reset() {
    key_map.clear();
}

bool KeyState::keyPressed(sf::Keyboard::Key key) {
    return key_map[key].key_down;
}

bool KeyState::keyReleased(sf::Keyboard::Key key) {
    return !key_map[key].key_down;
}

bool KeyState::keyJustPressed(sf::Keyboard::Key key) {
    KeyState& state = key_map[key];
    return state.key_down && state.update_tick == tick;
}

bool KeyState::keyJustReleased(sf::Keyboard::Key key) {
    KeyState& state = key_map[key];
    return !state.key_down && state.update_tick == tick;
}

bool KeyState::keyTyped(sf::Keyboard::Key key) {
    return key_map[key].type_tick == tick;
}