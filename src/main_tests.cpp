#ifdef TEST_MUTEX
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

// using namespace std::chrono_literals;

struct Player
{
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::thread thr;

    enum State
    {
        Stopped,
        Paused,
        Playing,
        Quit
    };

    State state;
    int counter;

    void signal_state(State st)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (st != state)
        {
            state = st;
            cv.notify_one();
        }
    }

    // main player monitor
    void monitor()
    {
        std::unique_lock<std::mutex> lock(mtx);
        bool bQuit = false;

        while (!bQuit)
        {
            switch (state)
            {
            case Playing:
                std::cout << ++counter << '.';
                cv.wait_for(lock, 200ms, [this]()
                            { return state != Playing; });
                break;

            case Stopped:
                cv.wait(lock, [this]()
                        { return state != Stopped; });
                std::cout << '\n';
                counter = 0;
                break;

            case Paused:
                cv.wait(lock, [this]()
                        { return state != Paused; });
                break;

            case Quit:
                bQuit = true;
                break;
            }
        }
    }

public:
    Player() : state(Stopped), counter(0)
    {
        thr = std::thread(std::bind(&Player::monitor, this));
    }

    ~Player()
    {
        quit();
        thr.join();
    }

    void stop() { signal_state(Stopped); }
    void play() { signal_state(Playing); }
    void pause() { signal_state(Paused); }
    void quit() { signal_state(Quit); }
};

void playertest()
{
    Player player;
    player.play();
    std::this_thread::sleep_for(2s);
    player.pause();
    std::this_thread::sleep_for(2s);
    player.play();
    std::this_thread::sleep_for(2s);
    player.stop();
    std::this_thread::sleep_for(2s);
    player.play();
    std::this_thread::sleep_for(2s);

    Misc::breakExit(0);
}
#endif // TEST_MUTEX

#ifdef EX_CONTEXT_SHARING
//========================================================================
// Context sharing example
// Copyright (c) Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

//#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

//#include "getopt.h"
//#include "linmath.h"

static const char *vertex_shader_text =
    "#version 110\n"
    "uniform mat4 MVP;\n"
    "attribute vec2 vPos;\n"
    "varying vec2 texcoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    texcoord = vPos;\n"
    "}\n";

static const char *fragment_shader_text =
    "#version 110\n"
    "uniform sampler2D texture;\n"
    "uniform vec3 color;\n"
    "varying vec2 texcoord;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(color * texture2D(texture, texcoord).rgb, 1.0);\n"
    "}\n";

struct vec2
{
    float x;
    float y;
};
struct vec3
{
    float x;
    float y;
    float z;
};

static const vec2 vertices[4] =
    {
        {0.f, 0.f},
        {1.f, 0.f},
        {1.f, 1.f},
        {0.f, 1.f}};

static void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main2(int argc, char **argv)
{
    // Glfw::initDefaultState();

    GLFWwindow *windows[2];
    GLuint texture, program, vertex_buffer;
    GLint mvp_location, vpos_location, color_location, texture_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        Misc::breakExit(EXIT_FAILURE);

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    windows[0] = glfwCreateWindow(400, 400, "First", NULL, NULL);
    if (!windows[0])
    {
        glfwTerminate();
        Misc::breakExit(EXIT_FAILURE);
    }
    //
    glfwSetInputMode(windows[0], GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(windows[0], GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(windows[0], GLFW_STICKY_MOUSE_BUTTONS, 1);
    //

    glfwSetKeyCallback(windows[0], key_callback);
    glfwMakeContextCurrent(windows[0]);

    //
    // glew
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "glewInit failed\n";
        Misc::breakExit(-1);
    }

    //

    // Only enable vsync for the first of the windows to be swapped to
    // avoid waiting out the interval for each window
    glfwSwapInterval(1);

    // The contexts are created with the same APIs so the function
    // pointers should be re-usable between them
    // gladLoadGL(glfwGetProcAddress);

    // Create the OpenGL objects inside the first context, created above
    // All objects will be shared with the second context, created below
    {
        int x, y;
        char pixels[16 * 16];
        GLuint vertex_shader, fragment_shader;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        srand((unsigned int)glfwGetTimerValue());

        for (y = 0; y < 16; y++)
        {
            for (x = 0; x < 16; x++)
                pixels[y * 16 + x] = rand() % 256;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 16, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
        glCompileShader(vertex_shader);

        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
        glCompileShader(fragment_shader);

        program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        mvp_location = glGetUniformLocation(program, "MVP");
        color_location = glGetUniformLocation(program, "color");
        texture_location = glGetUniformLocation(program, "texture");
        vpos_location = glGetAttribLocation(program, "vPos");

        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    glUseProgram(program);
    glUniform1i(texture_location, 0);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void *)0);

    windows[1] = glfwCreateWindow(400, 400, "Second", NULL, windows[0]);
    if (!windows[1])
    {
        glfwTerminate();
        Misc::breakExit(EXIT_FAILURE);
    }

    // Place the second window to the right of the first
    {
        int xpos, ypos, left, right, width;

        glfwGetWindowSize(windows[0], &width, NULL);
        glfwGetWindowFrameSize(windows[0], &left, NULL, &right, NULL);
        glfwGetWindowPos(windows[0], &xpos, &ypos);

        glfwSetWindowPos(windows[1], xpos + width + left + right, ypos);
    }

    glfwSetKeyCallback(windows[1], key_callback);

    glfwMakeContextCurrent(windows[1]);

    // While objects are shared, the global context state is not and will
    // need to be set up for each context

    glUseProgram(program);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void *)0);

    while (!glfwWindowShouldClose(windows[0]) &&
           !glfwWindowShouldClose(windows[1]))
    {
        int i;
        const vec3 colors[2] =
            {
                {0.8f, 0.4f, 1.f},
                {0.3f, 0.4f, 1.f}};

        for (i = 0; i < 2; i++)
        {
            int width, height;

            glfwGetFramebufferSize(windows[i], &width, &height);
            glfwMakeContextCurrent(windows[i]);

            glViewport(0, 0, width, height);

            Math::Matrix4 mvp;
            mvp.identity();
            // mat4x4_ortho(mvp, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat *)mvp.getData());
            glUniform3fv(color_location, 1, (float *)(&colors[i].x));
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glfwSwapBuffers(windows[i]);
        }

        glfwWaitEvents();
    }

    glfwTerminate();
    Misc::breakExit(EXIT_SUCCESS);
}
#endif // EX_CONTEXT_SHARING

#ifdef TEST_GL_CONTEXTS
void th_build_object(GLFWwindow *context, int n, std::list<Object *> *list, Obj3dPG *pg, std::mutex *mutex)
{
    D(__PRETTY_FUNCTION__ << "\n")
    std::thread::id threadID = std::this_thread::get_id();
    std::stringstream ss;
    ss << threadID;
    std::string thid = "[" + ss.str() + "]\t";
    glfwMakeContextCurrent(context);
    std::unique_lock<std::mutex> lock(*mutex);

    Obj3dBP bp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
    D(thid << "new bp: vao: " << bp.getVao() << "\t vbo: " << bp.getVboVertex() << "\n")
    Obj3d *o = new Obj3d(bp, *pg);
    o->local.setPos(Math::Vector3(n * 3, 0, 0));
    o->local.setScale(Math::Vector3(1, n * 3, 1));
    list->push_back(dynamic_cast<Object *>(o));
    bp.deleteVao();

    lock.unlock();
    while (1)
        ;
    // glfwSetWindowShouldClose(context, GLFW_TRUE);
    // glfwMakeContextCurrent(nullptr);
    // std::this_thread::sleep_for(3600s);
}
void scene_test_thread()
{
    float win_height = 900;
    float win_width = 1600;
    Glfw glfw(win_width, win_height);
    glfwSetWindowPos(glfw._window, 100, 50);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwFocusWindow(glfw._window);
    glfwSwapInterval(0);
    glfw.setTitle("Tests octree");
    GameManager m;
    m.glfw = &glfw;
    glfw.activateDefaultCallbacks(&m);

    Cam cam(glfw.getWidth(), glfw.getHeight());
    cam.local.setPos(0, 0, 8);
    Obj3dPG renderer(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
    Texture *tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");
    SkyboxPG rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);
    Skybox skybox(*tex_skybox, rendererSkybox);
    std::list<Object *> renderlistSkybox;
    renderlistSkybox.push_back(dynamic_cast<Object *>(&skybox));

    Obj3dBP::defaultSize = 1;
    Obj3dBP::defaultDataMode = BP_LINEAR;
    Obj3dBP::rescale = true;
    Obj3dBP::center = false;

    std::list<Object *> objlist;
    std::mutex mutex;

#define TESTTHREADSL
#ifdef TESTTHREADS
    GLFWwindow *contexts[M_THREADS_BUILDERS];
    std::thread *builders[M_THREADS_BUILDERS];
    for (size_t i = 0; i < M_THREADS_BUILDERS; i++)
    {
        contexts[i] = glfwCreateWindow(500, 30, std::to_string(i).c_str(), NULL, glfw._window);
        if (!contexts[i])
        {
            D("Error when creating context " << i << "\n")
            Misc::breakExit(5);
        }
        glfwSetWindowPos(contexts[i], 2000, 50 + 30 * i);
        builders[i] = new std::thread(std::bind(&th_build_object, contexts[i], i, &objlist, &renderer, &mutex));
    }
    // std::this_thread::sleep_for(5s);
    while (1)
    {
        if (mutex.try_lock())
        {
            D_(std::cout << objlist.size())
            if (objlist.size() == M_THREADS_BUILDERS)
            {
                mutex.unlock();
                break;
            }
            mutex.unlock();
            std::this_thread::sleep_for(1s);
        }
        else
        {
            D_(std::cout << ".")
        }
    }
    for (auto i : objlist)
    {
        D(i->getId() << "\n")
        Obj3d *o = dynamic_cast<Obj3d *>(i);
        if (!o)
        {
            D("dyn_cast failed to object: " << i << "\n")
            Misc::breakExit(88);
        }
        Obj3dBP &bp = o->getBlueprint();
        GLuint vao = bp.createVao();
        renderer.linkBuffersToVao(bp, vao);
    }

#else
    Obj3dBP bp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
    for (int n = 0; n < M_THREADS_BUILDERS; n++)
    {
        Obj3d *o = new Obj3d(bp, renderer);
        o->local.setPos(Math::Vector3(n * 3, 0, 0));
        o->local.setScale(Math::Vector3(1, n + 1, 1));
        objlist.push_back(dynamic_cast<Object *>(o));
    }
#endif // !TESTTHREADS

    Fps fps(25);
    unsigned int frames = 0;
    D("Begin rendering\n")
    while (!glfwWindowShouldClose(glfw._window))
    {
        D_(std::cout << "-")
        if (fps.wait_for_next_frame())
        {
            D_(std::cout << "1")
            frames++;
            if (frames % 500 == 0)
            {
                D(">>>>>>>>>>>>>>>>>>>> " << frames << " FRAMES <<<<<<<<<<<<<<<<<<<<\n")
                D("cam: " << cam.local.getPos() << "\n")
                D("objects: " << objlist.size() << "\n")
            }
            glfw.setTitle(std::to_string(fps.getFps()) + " fps");
            D_(std::cout << "2")
            glfwPollEvents();
            D_(std::cout << "3")
            glfw.updateMouse(); // to do before cam's events
            D_(std::cout << "4")
            cam.events(glfw, float(fps.getTick()));
            D_(std::cout << "5")

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.renderObjects(objlist, cam, PG_FORCE_DRAW); // PG_FRUSTUM_CULLING
            D_(std::cout << "6")
            rendererSkybox.renderObjects(renderlistSkybox, cam, PG_FORCE_DRAW);
            D_(std::cout << "7")

            glfwSwapBuffers(glfw._window);

            if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_ESCAPE))
                glfwSetWindowShouldClose(glfw._window, GLFW_TRUE);
            D_(std::cout << "f")
        }
    }
    D_(std::cout << "end\n")
}
#endif // TEST_GL_CONTEXTS

#ifdef TEXT_TEST
#include "misc.hpp"

// shader
#include <iostream>
#include <map>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            // if geometry shader path is present, also load a geometry shader
            if (geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure &e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if (geometryPath != nullptr)
        {
            const char *gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryPath != nullptr)
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr)
            glDeleteShader(geometry);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

//#include <learnopengl/shader.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    //{
    //	std::cout << "Failed to initialize GLAD" << std::endl;
    //	return -1;
    //}

    // glew
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "glewInit failed\n";
        Misc::breakExit(-1);
    }

    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // compile and setup the shader
    // ----------------------------
    Shader shader("SimpleGL/shaders/text.vs.glsl", "SimpleGL/shaders/text.fs.glsl");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // find path to font
    std::string font_name = Misc::getCurrentDirectory() + "SimpleGL/fonts/arial.ttf"; // FileSystem("resources/fonts/Antonio-Bold.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else
    {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer);
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)};
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        RenderText(shader, "This is sample text", 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
        RenderText(shader, "(C) LearnOpenGL.com", 540.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// render line of text
// -------------------
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},

            {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}};
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
#endif // TEXT_TEST

#ifdef POE
void test_poe()
{
    float crit_chance = 1;
    float crit_multi = 5;
    float min_frenzy = 0;
    float max_frenzy = 9;
    float frenzy_dmg_bonus = 0.05;
    float chance_frenzy_on_crit = 0.4;
    float chance_max_frenzy = 0.15;
    float add_arrow = 3;      // 2deadeye + 1quiver
    float barrage_arrows = 3; // 3 for barrage supp, 4 for barrage attack
    float base_damage = 1000;

    // conf print
    std::cout << "% crit              \t" << (crit_chance * 100) << "%\n";
    std::cout << "% crit_multi        \t" << (crit_multi * 100) << "%\n";
    std::cout << "minimum frenzy      \t" << min_frenzy << "\n";
    std::cout << "maximum frenzy      \t" << max_frenzy << "\n";
    std::cout << "% frenzy on crit    \t" << (chance_frenzy_on_crit * 100) << "%\n";
    std::cout << "add. arrows         \t" << add_arrow << "\n";
    std::cout << "barrage add. arrows \t" << barrage_arrows << "\n";
    std::cout << "base damage         \t" << base_damage << "\n\n";
    std::cout << "context : single target\n";
    std::cout << "not counting chance to get max frenzy, or frenzy reset on reaching max frenzy\n";
    std::cout << "bows have same base damage\n";
    std::cout << "assuming we are max frenzy for the usual bow attack\n";
    std::cout << "assuming we reached max frenzy for the gluttonous tide attack\n";
    std::cout << "\n";

    float final_damage_usual = 0;
    {
        // usual bow always max frenzy
        float bonus_damage = (1 + crit_chance) * crit_multi * (1 + max_frenzy * frenzy_dmg_bonus);
        float total_arrows = (1 + barrage_arrows + add_arrow);
        final_damage_usual = base_damage * total_arrows * bonus_damage;
        std::cout << "average usual bow damage : " << final_damage_usual << "\n";
    }
    float final_damage_gluttonous = 0;
    {
        // the gluttonous tide, assuming we reached max frenzy
        float total_arrows_base = (1 + barrage_arrows + add_arrow);
        float total_arrows = total_arrows_base + max_frenzy;
        float chance_frenzy_on_hit = chance_frenzy_on_crit * crit_chance;

        float total_damage = 0;
        for (size_t i = 0; i < total_arrows; i++)
        { // 1 barrage(d) attack
            float base_frenzy = min_frenzy + i;
            float average_frenzy_bonus_dmg = base_frenzy * chance_frenzy_on_hit * frenzy_dmg_bonus;
            float average_multi_bonus = std::pow(chance_frenzy_on_hit, base_frenzy) * 0.45;
            float bonus_crit_damage = (1 + crit_chance) * (crit_multi + base_frenzy * average_multi_bonus);
            float arrow_damage = base_damage * bonus_crit_damage * (1 + average_frenzy_bonus_dmg);
            final_damage_gluttonous += arrow_damage;
        }
        std::cout << "average weird bow damage : " << final_damage_gluttonous << "\n";
    }
    float bonus_damage = final_damage_gluttonous / final_damage_usual - 1;
    std::cout << "bonus damage : " << (bonus_damage * 100) << " %\n";
    Misc::breakExit(0);
    /*
        dmg * (1 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4)
        dmg * (1 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4 + bonus*0.4)

        ---
    */
}

#endif // POE

#ifdef TEST_GL_MEMORY_TEXTURE
class TestGlMem
{
public:
    TestGlMem()
    {
        tex = nullptr;
        panel = nullptr;

        // tex = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");
        tex = new Texture(SIMPLEGL_FOLDER + "images/skybox4096.bmp");
        if (tex)
        {
            // panel = new UIImage(tex);
        }
        std::cout << "data allocated.\n";
    }
    ~TestGlMem()
    {
        if (tex)
            delete tex;
        if (panel)
            delete panel;
        std::cout << "data deallocated.\n";
    }
    Texture *tex;
    UIPanel *panel;

private:
};
void th_test_momory_opengl(TestGlMem **datas, size_t size)
{
    std::string out;
    std::cout << "Press ENTER to deallocate data.\n";
    std::cin >> out;
    for (size_t j = 0; j < size; j++)
    {
        datas[j]->tex->unloadFromGPU();
        // delete datas[j];
    }
    // delete[] datas;
    glFinish();
    std::cout << "Thread exiting.\n";
}
void test_memory_opengl()
{
    std::string out;
    Glfw gl(800, 600);
    glfwSetWindowPos(gl._window, 100, 50);
    glfwSwapInterval(0); // 0 = disable vsynx
    gl.setTitle("Tests memory opengl");
    std::this_thread::sleep_for(2s);

    size_t size = 20;
    std::cout << "Press ENTER to allocate data.\n";
    // std::cin >> out;
    TestGlMem **datas = new TestGlMem *[size];
    for (size_t j = 0; j < size; j++)
    {
        datas[j] = new TestGlMem();
    }

    for (size_t i = 0; i < 1; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            datas[j]->tex->loadToGPU();
        }
        std::this_thread::sleep_for(200ms);
        if (0)
        {
            th_test_momory_opengl(datas, size);
        }
        else
        {
            std::thread thread = std::thread(std::bind(&th_test_momory_opengl, datas, size));
            thread.join();
            std::cout << "Thread joined.\n";
        }
    }

    std::cout << "Press ENTER to exit program.\n";
    std::cin >> out;
    std::exit(0);
}
#endif // TEST_GL_MEMORY_TEXTURE

#ifdef TEST_GL_MEMORY_OBJ3DBP
Obj3dBP *allocate_bp(std::string filename)
{
    std::string out;
    std::cout << "Type something to allocate data.\n";
    std::cin >> out;
    return new Obj3dBP(filename);
}
void deallocate_bp(Obj3dBP *bp)
{
    std::string out;
    std::cout << "Type something to deallocate data.\n";
    std::cin >> out;
    delete bp;
    std::cout << "BP deallocated.\n";
}
void test_memory_opengl_obj3dbp()
{
    Glfw gl(800, 600);
    glfwSetWindowPos(gl._window, 100, 50);
    glfwSwapInterval(0); // 0 = disable vsynx
    gl.setTitle("Tests memory opengl Obj3dBP");
    std::this_thread::sleep_for(2s);

    // Obj3dPG		rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
    // Blueprint global settings
    Obj3dBP::defaultSize = 1;
    Obj3dBP::defaultDataMode = BP_LINEAR;
    Obj3dBP::rescale = true;
    Obj3dBP::center = false;

    Obj3dBP *bp_arsenal = allocate_bp(SIMPLEGL_FOLDER + "obj3d/ARSENAL_VG33/Arsenal_VG33.obj");
    deallocate_bp(bp_arsenal);

    std::thread thread = std::thread(std::bind(&deallocate_bp, bp_arsenal));
    thread.join();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(gl._window);
    glFlush();
    glFinish();

    std::string out;
    std::cout << "Type something to exit program.\n";
    std::cin >> out;
    std::exit(0);
}
#endif // TEST_GL_MEMORY_OBJ3DBP
