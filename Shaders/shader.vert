#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform MVP {
    layout (offset = 0) mat4 model;
    layout (offset = 64) mat4 view;
    layout (offset = 128) mat4 proj;
} mvp;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPos, 1.0);
    fragColor = inColor;
}
