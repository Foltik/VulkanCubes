#pragma once

#include "../Context.h"

class Engine;

class Frame {
public:
    explicit Frame(Engine& engine) : m_engine(engine) {}
    virtual ~Frame() = default;

    virtual void update(float dt, Context& context) = 0;
    virtual void render(Context& context) = 0;

    virtual void enter() = 0;
    virtual void leave() = 0;

protected:
    Engine& m_engine;
};
