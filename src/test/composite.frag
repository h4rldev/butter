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
  vec2 p = gl_FragCoord.xy;
  vec4 color = texture(u_texture, p / push.extent);

  vec2 half_size = push.region.zw * 0.5;
  vec2 center = push.region.xy + half_size;
  vec2 q = abs(p - center) - half_size + vec2(push.radius);

  float dist = length(max(q, 0.0)) - push.radius;
  float mask = 1.0 - smoothstep(-1.0, 1.0, dist);

  out_color = vec4(color.rgb, color.a * mask);
}