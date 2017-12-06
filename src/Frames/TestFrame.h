#pragma once

#include "Frame.h"

class TestFrame : public Frame {
public:
    explicit TestFrame(Engine& engine);

    void update(float dt, Context& context) override;

    void render(Context& context) override;

    void enter() override;

    void leave() override;

private:
    void gen();
    void mesh();

    glm::vec3 cameraPos{2.0f, 2.0f, 2.0f};
    glm::vec2 cameraAngles{225.0f, -35.0f};
    glm::vec3 cameraTarget{0.0f, 0.0f, 0.0f};

    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    float sens = 0.05;
    float speed = 0.0005;

    Context::MVP mvp = {};

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    struct VoxelFace {
        bool transparent;
        int type;
        int side;

        bool operator==(VoxelFace& other) {
            return transparent == other.transparent &&
                   type == other.type;
        }
        bool operator!=(VoxelFace& other) {
            return transparent != other.transparent &&
                   type != other.type;
        }
    };

    std::array<std::array<std::array<VoxelFace, 3>, 3>, 3> voxels {};

    static constexpr int SOUTH = 0;
    static constexpr int NORTH = 1;
    static constexpr int EAST = 2;
    static constexpr int WEST = 3;
    static constexpr int TOP = 4;
    static constexpr int BOTTOM = 5;

    std::vector<float> cubeVerts {
        -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, // 0 back left top
        0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // 1 back right top
        0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,   // 2 back right bottom
        -1.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,  // 3 back left bottom
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 4 front left top
        0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 5 front right top
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // 6 front right bottom
        -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f    // 7 front left bottom
    };

    std::vector<uint16_t> cubeIndices = {
        2, 1, 0, 0, 3, 2, // back
        4, 5, 6, 6, 7, 4, // front
        3, 0, 4, 4, 7, 3, // left
        5, 1, 2, 2, 6, 5, // right
        4, 0, 1, 1, 5, 4, // top
        2, 3, 7, 7, 6, 2  // bottom
    };

    std::vector<float> verts {};
    std::vector<uint16_t> indices {};
    int count = 1;

    float lastIntegral = 0.0f;
    float frameTime = 0.0f;
    int frameCount;
};


