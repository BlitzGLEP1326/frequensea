-- Visualize FFT data as a texture

VERTEX_SHADER = [[
#ifdef GL_ES
attribute vec3 vp;
attribute vec3 vn;
attribute vec2 vt;

varying vec3 color;
varying vec2 texCoord;

#else 
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
out vec3 color;
out vec2 texCoord;
#endif
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    texCoord = vt;
    gl_Position = vec4(vp.x, vp.z, 0, 1.0);
}
]]

FRAGMENT_SHADER = [[
#ifdef GL_ES
precision mediump float;
varying vec3 color;
varying vec2 texCoord;

#else
#version 400
in vec3 color;
in vec2 texCoord;
#endif
uniform sampler2D uTexture;
void main() {
    float r = texture(uTexture, texCoord).r;
    float g = texture(uTexture, texCoord).g;
    float pwr = r * r + g * g;
    float pwr_dbfs = 10.0 * log2(pwr + 1.0e-20) / log2(2.7182818284);

    //float v = sqrt(r * r + g * g) * 0.1;
    float v = pwr_dbfs * 0.02;
    fragColor = vec4(v, v, v, 0.95);
}
]]

function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(1024, 1024)

    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, fft_buffer, 1024, 1024)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
