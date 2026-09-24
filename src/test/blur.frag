#version 450

layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform Push {
  vec4 region;
  vec2 extent;
  vec2 dir;
  float blur;
  float radius;
  vec2 pad;
} push;

void main() {
  vec2 uv = gl_FragCoord.xy / push.extent;
  vec2 step = push.dir / push.extent;

  int radius = int(ceil(push.blur));
  float sigma = max(push.blur * 0.5, 0.5);

  vec4 sum = vec4(0.0);
  float weight_sum = 0.0;

  for (int i = -radius; i <= radius; i++) {
    float w = exp(-float(i * i) / (2.0 * sigma * sigma));
    sum += texture(u_texture, uv + step * float(i)) * w;
    weight_sum += w;
  }

  out_color = sum / weight_sum;
}