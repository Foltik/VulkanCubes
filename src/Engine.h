#pragma once
#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Frames/Frame.h"
#include "Context.h"
#include "Window.h"

#ifdef NDEBUG
constexpr bool g_debug = false;
#else
constexpr bool g_debug = true;
#endif


class Engine {
public:
    Engine();
    ~Engine();

    void launch();

public:
    template <typename T, typename... Args>
    void pushFrame(Args&&... args) {
        if (!m_frames.empty())
            getFrame().leave();

        m_frames.push_back(std::make_unique<T>(std::forward<Args>(args)...));

        getFrame().enter();
    }

    void popFrame();

    template <typename T, typename... Args>
    void changeFrame(Args&&... args) {
        if (!m_frames.empty()) {
            getFrame().leave();
            popFrame();
        }

        m_frames.push_back(std::make_unique<T>(std::forward<Args>(args)...));

        getFrame().enter();
    };
    Frame& getFrame();



private:
    Window window;
    Context context;

    std::vector<std::unique_ptr<Frame>> m_frames;
};


