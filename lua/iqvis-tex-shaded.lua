-- Visualize IQ data as a texture from the HackRF
-- Calculate normals and lighting

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
flat out vec4 color;
out vec2 texCoord;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
uniform sampler2D uTexture;
void main() {
    float d = 0.9;
    float t1 = texture(uTexture, vt).r * d;
    float t2 = texture(uTexture, vt + vec2(0.01, 0)).r * d;
    float t3 = texture(uTexture, vt + vec2(0, 0.01)).r * d;
    vec3 v1 = vec3(vp.x, t1, vp.z);
    vec3 v2 = vec3(vp.x + 0.01, t2, vp.z);
    vec3 v3 = vec3(vp.x, t3, vp.z + 0.01);

    vec3 u = v2 - v1;
    vec3 v = v3 - v1;
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    vec3 n = vec3(x, y, z);

    color = vec4(1.0, 1.0, 1.0, 0.95) * dot(normalize(v1), normalize(n)) * 0.5;
    color += vec4(0.2, 0.2, 0.4, 1.0);
    color *= 2;

    float l = 1.0 - ((vp.x * vp.x + vp.z * vp.z) * 2.0);
    color *= vec4(l, l, l, l);
    texCoord = vt;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(v1, 1.0);
}
]]

FRAGMENT_SHADER = [[
#version 400
flat in vec4 color;
in vec2 texCoord;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = color;
}
]]

function setup()
    freq = 2412
    device = nrf_start(freq, "../rfdata/rf-202.500-2.raw")
    camera = ngl_camera_init_look_at(0, 1.0, 1.0)
    shader = ngl_shader_init(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_create(shader, "uTexture")
    model = ngl_model_init_grid_triangles(100, 100, 0.01, 0.01)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, GL_RED, 512, 512, device.samples)
    ngl_draw_model(camera, model, shader)

    --nrf_freq_set(device, freq)
    --freq = freq + 0.1
end
