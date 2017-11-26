#pragma once

class Frame {
public:
    virtual void enter() = 0;
    virtual void leave() = 0;
};
