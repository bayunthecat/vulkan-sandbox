#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 colors;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
  mat4 model = ubo.model;
  model[3].x += 5.0 * gl_InstanceIndex;
  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  fragColor = colors;
  fragTexCoord = inTexCoord;
}
