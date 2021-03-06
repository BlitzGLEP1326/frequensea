// Visualisation on the Raspberry PI

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <rtl-sdr.h>

#define WIDTH 256
#define HEIGHT 256

GLFWwindow* window;
GLuint texture_id;
GLuint program;
GLuint position_vbo;
GLuint uv_vbo;
GLuint vao;
GLfloat buffer[WIDTH * HEIGHT * 3];
rtlsdr_dev_t *device;
pthread_t receive_thread;
double freq_mhz = 124.2;
int paused = 0;
float intensity = 0.03;
void *rtl_buffer;
const uint32_t rtl_buffer_length = (16 * 16384);
int rtl_should_quit = 0;

void rtl_check_status(rtlsdr_dev_t *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "RTL-SDR: %s (Status code %d) %s:%d\n", message, status, file, line);
        if (device != NULL) {
            rtlsdr_close(device);
        }
        exit(EXIT_FAILURE);
    }
}

#define RTL_CHECK_STATUS(device, status, message) rtl_check_status(device, status, message, __FILE__, __LINE__)

void receive_block(unsigned char *in_buffer, uint32_t buffer_length, void *ctx) {
    if (paused) return;
    memset(buffer, 0, WIDTH * HEIGHT * 3 * sizeof(GLfloat));
    for (int i = 0; i < buffer_length; i += 2) {
        int vi = in_buffer[i];
        int vq = in_buffer[i + 1];

        int d = (vq * 256) + vi;
        float v = buffer[d];
        v += intensity;
        v = v < 0 ? 0 : v > 255 ? 255 : v;
        buffer[d * 3] = v;
    }
}

// This function will block, so needs to be called on its own thread.
void *_receive_loop(rtlsdr_dev_t *device) {
    while (!rtl_should_quit) {
        int n_read;
        int status = rtlsdr_read_sync(device, rtl_buffer, rtl_buffer_length, &n_read);
        RTL_CHECK_STATUS(device, status, "rtlsdr_read_sync");

        if (n_read < rtl_buffer_length) {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            exit(EXIT_FAILURE);
        }

        receive_block(rtl_buffer, rtl_buffer_length, device);
    }
    return NULL;
}

static void setup_rtl() {
    int status;

    rtl_buffer = calloc(rtl_buffer_length, sizeof(uint8_t));

    int device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        fprintf(stderr, "RTL-SDR: No devices found.\n");
        exit(EXIT_FAILURE);
    }

    const char *device_name = rtlsdr_get_device_name(0);
    printf("Device %s\n", device_name);

    status = rtlsdr_open(&device, 0);
    RTL_CHECK_STATUS(device, status, "rtlsdr_open");

    status = rtlsdr_set_sample_rate(device, 2e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");

    // Set auto-gain mode
    status = rtlsdr_set_tuner_gain_mode(device, 0);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_tuner_gain_mode");

    status = rtlsdr_set_agc_mode(device, 1);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_agc_mode");

    status = rtlsdr_set_center_freq(device, freq_mhz * 1e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");

    status = rtlsdr_reset_buffer(device);
    RTL_CHECK_STATUS(device, status, "rtlsdr_reset_buffer");

    printf("Start\n");
    pthread_create(&receive_thread, NULL, (void *(*)(void *))_receive_loop, device);
    printf("Running\n");

}

static void set_frequency() {
    freq_mhz = round(freq_mhz * 10.0) / 10.0;
    printf("Seting freq to %f MHz.\n", freq_mhz);
    int status = rtlsdr_set_center_freq(device, freq_mhz * 1e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");
}

static void teardown_rtl() {
    int status;

    rtl_should_quit = 1;

    printf("pthread_join\n");
    pthread_join(receive_thread, NULL);

    printf("rtlsdr_close\n");
    status = rtlsdr_close(device);
    //printf("Closed\n");
    RTL_CHECK_STATUS(device, status, "rtlsdr_close");
}


void ngl_check_gl_error(const char *file, int line) {
    GLenum err = glGetError();
    int has_error = 0;
    while (err != GL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch(err) {
            case GL_INVALID_OPERATION:
            msg = "GL_INVALID_OPERATION";
            break;
            case GL_INVALID_ENUM:
            msg = "GL_INVALID_ENUM";
            fprintf(stderr, "OpenGL error: GL_INVALID_ENUM\n");
            break;
            case GL_INVALID_VALUE:
            msg = "GL_INVALID_VALUE";
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
            break;
            case GL_OUT_OF_MEMORY:
            msg = "GL_OUT_OF_MEMORY";
            fprintf(stderr, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
            default:
            msg = "UNKNOWN_ERROR";
        }
        fprintf(stderr, "OpenGL error: %s - %s:%d\n", msg, file, line);
        err = glGetError();
    }
    if (has_error) {
        exit(EXIT_FAILURE);
    }
}

#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)

static void check_shader_error(GLuint shader) {
    int length = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        char message[length];
        glGetShaderInfoLog(shader, length, NULL, message);
        printf("%s\n", message);
    }
}


static const GLfloat positions[] = {
    -1.0, -1.0,
     1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0
};

static const GLfloat uvs[] = {
    1.0, 1.0,
    1.0, 0.0,
    0.0, 1.0,
    0.0, 0.0
};

const int ATTRIB_VERTEX = 0;
const int ATTRIB_TEXTUREPOSITION = 1;

static void setup() {
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, positions);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITION, 2, GL_FLOAT, 0, 0, uvs);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITION);
    NGL_CHECK_ERROR();

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    NGL_CHECK_ERROR();

    const char *vertex_shader_source =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec2 vp;\n"
        "attribute vec2 vt;\n"
        "varying vec2 uv;\n"
        "void main(void) {\n"
        "  uv = vt;\n"
        "  gl_Position = vec4(vp.x, vp.y, 0, 1);\n"
        "}\n";

    const char *fragment_shader_source =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D texture;\n"
        "varying vec2 uv;\n"
        "void main(void) {\n"
        "  vec4 c = texture2D(texture, uv);\n"
        "  float v = c.r;\n"
        "  gl_FragColor = vec4(v, v, v, 1);\n"
        "}\n";

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_error(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_error(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    NGL_CHECK_ERROR();

    glActiveTexture(0);
    GLuint u_texture = glGetUniformLocation(program, "texture");
    glUniform1i(u_texture, texture_id);
    NGL_CHECK_ERROR();
}

static void prepare() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    NGL_CHECK_ERROR();
}

static void update() {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, buffer);
    NGL_CHECK_ERROR();
}

static void draw() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_SRC_COLOR);
    NGL_CHECK_ERROR();

    glUseProgram(program);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glUseProgram(0);
}

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (mods == 0) {
            freq_mhz += 0.1;
        } else {
            freq_mhz += 10;
        }
        set_frequency();
    } else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (mods == 0) {
            freq_mhz -= 0.1;
        } else {
            freq_mhz -= 10;
        }
        set_frequency();
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        paused = !paused;
    } else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        intensity += 0.01;
        printf("Intensity: %.2f\n", intensity);
    } else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        intensity -= 0.01;
        printf("Intensity: %.2f\n", intensity);
    }
}

int main(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    window = glfwCreateWindow(WIDTH * 2, HEIGHT * 2, "RTL-SDR", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    setup_rtl();
    setup();
    while (!glfwWindowShouldClose(window)) {
        prepare();
        update();
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    teardown_rtl();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
