#include <compiler_settings.h>

#if 0 // test mutex
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

//using namespace std::chrono_literals;

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
				cv.wait_for(lock, 200ms, [this]() { return state != Playing; });
				break;

			case Stopped:
				cv.wait(lock, [this]() { return state != Stopped; });
				std::cout << '\n';
				counter = 0;
				break;

			case Paused:
				cv.wait(lock, [this]() { return state != Paused; });
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

void	playertest() {
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

	std::exit(0);
}
#endif // test mutex

#if 0 //example context sharing
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

static const char* vertex_shader_text =
"#version 110\n"
"uniform mat4 MVP;\n"
"attribute vec2 vPos;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    texcoord = vPos;\n"
"}\n";

static const char* fragment_shader_text =
"#version 110\n"
"uniform sampler2D texture;\n"
"uniform vec3 color;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color * texture2D(texture, texcoord).rgb, 1.0);\n"
"}\n";

struct vec2 {
	float x;
	float y;
};
struct vec3 {
	float x;
	float y;
	float z;
};

static const vec2 vertices[4] =
{
	{ 0.f, 0.f },
	{ 1.f, 0.f },
	{ 1.f, 1.f },
	{ 0.f, 1.f }
};

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main2(int argc, char** argv)
{
	//Glfw::initDefaultState();

	GLFWwindow* windows[2];
	GLuint texture, program, vertex_buffer;
	GLint mvp_location, vpos_location, color_location, texture_location;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		std::exit(EXIT_FAILURE);

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	windows[0] = glfwCreateWindow(400, 400, "First", NULL, NULL);
	if (!windows[0])
	{
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}
	//
	glfwSetInputMode(windows[0], GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetInputMode(windows[0], GLFW_STICKY_KEYS, 1);
	glfwSetInputMode(windows[0], GLFW_STICKY_MOUSE_BUTTONS, 1);
	//

	glfwSetKeyCallback(windows[0], key_callback);
	glfwMakeContextCurrent(windows[0]);

	//
	//glew
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "glewInit failed\n";
		Misc::breakExit(-1);
	}

	//

	// Only enable vsync for the first of the windows to be swapped to
	// avoid waiting out the interval for each window
	glfwSwapInterval(1);

	// The contexts are created with the same APIs so the function
	// pointers should be re-usable between them
	//gladLoadGL(glfwGetProcAddress);

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
		sizeof(vertices[0]), (void*)0);

	windows[1] = glfwCreateWindow(400, 400, "Second", NULL, windows[0]);
	if (!windows[1])
	{
		glfwTerminate();
		std::exit(EXIT_FAILURE);
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
		sizeof(vertices[0]), (void*)0);

	while (!glfwWindowShouldClose(windows[0]) &&
		!glfwWindowShouldClose(windows[1]))
	{
		int i;
		const vec3 colors[2] =
		{
			{ 0.8f, 0.4f, 1.f },
			{ 0.3f, 0.4f, 1.f }
		};

		for (i = 0; i < 2; i++)
		{
			int width, height;

			glfwGetFramebufferSize(windows[i], &width, &height);
			glfwMakeContextCurrent(windows[i]);

			glViewport(0, 0, width, height);

			Math::Matrix4	mvp;
			mvp.identity();
			//mat4x4_ortho(mvp, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
			glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp.getData());
			glUniform3fv(color_location, 1, (float*)(&colors[i].x));
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glfwSwapBuffers(windows[i]);
		}

		glfwWaitEvents();
	}

	glfwTerminate();
	std::exit(EXIT_SUCCESS);
}
#endif ////example context sharing

#if 0 //text test

#include "misc.hpp"

//shader
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
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
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
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
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
			const char* gShaderCode = geometryCode.c_str();
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
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const
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
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2   Size;      // Size of glyph
	glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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
	//if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	//{
	//	std::cout << "Failed to initialize GLAD" << std::endl;
	//	return -1;
	//}

	//glew
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
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
	if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return -1;
	}
	else {
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
				face->glyph->bitmap.buffer
			);
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
				static_cast<unsigned int>(face->glyph->advance.x)
			};
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
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


// render line of text
// -------------------
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
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
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
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
#endif//text test

#if 1
#include "simplegl.h"
#include "trees.h"
#include <typeinfo>

void	test_poe() {
	float	crit_chance = 1;
	float	crit_multi = 5;
	float	min_frenzy = 0;
	float	max_frenzy = 9;
	float	frenzy_dmg_bonus = 0.05;
	float	chance_frenzy_on_crit = 0.4;
	float	chance_max_frenzy = 0.15;
	float	add_arrow = 3; //2deadeye + 1quiver
	float	barrage_arrows = 3;//3 for barrage supp, 4 for barrage attack
	float	base_damage = 1000;

	//conf print
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

	float	final_damage_usual = 0;
	{
		//usual bow always max frenzy
		float	bonus_damage = (1 + crit_chance) * crit_multi * (1 + max_frenzy * frenzy_dmg_bonus);
		float	total_arrows = (1 + barrage_arrows + add_arrow);
		final_damage_usual = base_damage * total_arrows * bonus_damage;
		std::cout << "average usual bow damage : " << final_damage_usual << "\n";
	}
	float	final_damage_gluttonous = 0;
	{
		// the gluttonous tide, assuming we reached max frenzy
		float	total_arrows_base = (1 + barrage_arrows + add_arrow);
		float	total_arrows = total_arrows_base + max_frenzy;
		float	chance_frenzy_on_hit = chance_frenzy_on_crit * crit_chance;

		float	total_damage = 0;
		for (size_t i = 0; i < total_arrows; i++) {// 1 barrage(d) attack
			float	base_frenzy = min_frenzy + i;
			float	average_frenzy_bonus_dmg = base_frenzy * chance_frenzy_on_hit * frenzy_dmg_bonus;
			float	average_multi_bonus = std::pow(chance_frenzy_on_hit, base_frenzy) * 0.45;
			float	bonus_crit_damage = (1 + crit_chance) * (crit_multi + base_frenzy * average_multi_bonus);
			float	arrow_damage = base_damage * bonus_crit_damage * (1 + average_frenzy_bonus_dmg);
			final_damage_gluttonous += arrow_damage;
		}
		std::cout << "average weird bow damage : " << final_damage_gluttonous << "\n";
	}
	float	bonus_damage = final_damage_gluttonous / final_damage_usual - 1;
	std::cout << "bonus damage : " << (bonus_damage * 100) << " %\n";
	std::exit(0);
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

void	pdebug(bool reset = false) {
	const char* s = "0123456789`~!@#$%^&*()_+-=[]{}\\|;:'\",./<>?";
	static unsigned int i = 0;
	std::cout << s[i];
	i++;
	if (reset || i == 42)
		i = 0;
}

void	blitToWindow(FrameBuffer* readFramebuffer, GLenum attachmentPoint, UIPanel* panel) {
	GLuint fbo;
	if (readFramebuffer) {
		fbo = readFramebuffer->fbo;
	}
	else {
		fbo = panel->getFbo();
	}
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	//glViewport(0, 0, manager->glfw->getWidth(), manager->glfw->getHeight());//size of the window/image or panel width ?
	glReadBuffer(attachmentPoint);
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	int w;
	int h;
	if (readFramebuffer) {
		w = readFramebuffer->getWidth();
		h = readFramebuffer->getHeight();
	}
	else if (panel->getTexture()) {
		w = panel->getTexture()->getWidth();
		h = panel->getTexture()->getHeight();
	}
	else {
		std::cout << "FUCK " << __PRETTY_FUNCTION__ << "\n";
		std::exit(2);
	}
	if (0) {
		std::cout << "copy " << w << "x" << h << "\tresized\t" << panel->_width << "x" << panel->_height \
			<< "\tat pos\t" << panel->_posX << ":" << panel->_posY << "\n";
		// << " -> " << (panel->posX + panel->width) << "x" << (panel->posY + panel->height) << "\n";
	}
	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void	check_paddings() {
	//	std::cout << sizeof(BITMAPINFOHEADER) << " = " << sizeof(BMPINFOHEADER) << "\n";
#ifdef _WIN322
	std::cout << sizeof(BITMAPFILEHEADER) << " = " << sizeof(BMPFILEHEADER) << "\n";
	std::cout << "bfType\t" << offsetof(BMPINFOHEADERBITMAPFILEHEADER, bfType) << "\n";
	std::cout << "bfSize\t" << offsetof(BITMAPFILEHEADER, bfSize) << "\n";
	std::cout << "bfReserved1\t" << offsetof(BITMAPFILEHEADER, bfReserved1) << "\n";
	std::cout << "bfReserved2\t" << offsetof(BITMAPFILEHEADER, bfReserved2) << "\n";
	std::cout << "bfOffBits\t" << offsetof(BITMAPFILEHEADER, bfOffBits) << "\n";
#endif//_WIN32
	std::cout << "unsigned short\t" << sizeof(unsigned short) << "\n";
	std::cout << "unsigned long \t" << sizeof(unsigned long) << "\n";
	std::cout << "long          \t" << sizeof(long) << "\n";
	std::cout << "long long     \t" << sizeof(long long) << "\n";
	std::cout << "int           \t" << sizeof(int) << "\n";
	if ((sizeof(BMPFILEHEADER) != 14) || (sizeof(BMPINFOHEADER) != 40)) {
		cerr << "Padding in structure, exiting...\n" << "\n";
		std::cout << "BMPFILEHEADER\t" << sizeof(BMPFILEHEADER) << "\n";
		std::cout << "bfType     \t" << offsetof(BMPFILEHEADER, bfType) << "\n";
		std::cout << "bfSize     \t" << offsetof(BMPFILEHEADER, bfSize) << "\n";
		std::cout << "bfReserved1\t" << offsetof(BMPFILEHEADER, bfReserved1) << "\n";
		std::cout << "bfReserved2\t" << offsetof(BMPFILEHEADER, bfReserved2) << "\n";
		std::cout << "bfOffBits\t" << offsetof(BMPFILEHEADER, bfOffBits) << "\n";
		std::cout << "-----\n";
		std::cout << "BMPINFOHEADER\t" << sizeof(BMPINFOHEADER) << "\n";
		std::cout << "biSize     \t" << offsetof(BMPINFOHEADER, biSize) << "\n";
		std::cout << "biWidth    \t" << offsetof(BMPINFOHEADER, biWidth) << "\n";
		std::cout << "biHeight\t" << offsetof(BMPINFOHEADER, biHeight) << "\n";
		std::cout << "biPlanes\t" << offsetof(BMPINFOHEADER, biPlanes) << "\n";
		std::cout << "biBitCount\t" << offsetof(BMPINFOHEADER, biBitCount) << "\n";
		std::cout << "biCompression\t" << offsetof(BMPINFOHEADER, biCompression) << "\n";
		std::cout << "biSizeImage\t" << offsetof(BMPINFOHEADER, biSizeImage) << "\n";
		std::cout << "biXPelsPerMeter\t" << offsetof(BMPINFOHEADER, biXPelsPerMeter) << "\n";
		std::cout << "biYPelsPerMeter\t" << offsetof(BMPINFOHEADER, biYPelsPerMeter) << "\n";
		std::cout << "biClrUsed\t" << offsetof(BMPINFOHEADER, biClrUsed) << "\n";
		std::cout << "biClrImportant\t" << offsetof(BMPINFOHEADER, biClrImportant) << "\n";
		std::exit(ERROR_PADDING);
	}
}

class AnchorCameraBH : public Behavior
{
	/*
		La rotation fonctionne bien sur la cam (ca a l'air),
		mais le probleme vient de l'ordre de rotation sur l'anchor.
		? Ne pas utiliser ce simple system de rotation
		? rotater par rapport au system local de l'avion
		? se baser sur une matrice cam-point-at
			https://mikro.naprvyraz.sk/docs/Coding/Atari/Maggie/3DCAM.TXT
	*/
public:
	AnchorCameraBH() : Behavior() {
		this->copyRotation = true;
	}
	void	behaveOnTarget(BehaviorManaged* target) {
		if (this->_anchor) {
			Object* speAnchor = dynamic_cast<Object*>(this->_anchor);//specialisation part
			// turn this in Obj3d to get the BP, to get the size of the ovject,
			// to position the camera relatively to the obj's size.

			Cam* speTarget = dynamic_cast<Cam*>(target);//specialisation part

			Math::Vector3	forward(0, -15, -35);
			if (this->copyRotation) {
				Math::Rotation	rot = speAnchor->local.getRot();
				forward.rotate(rot, 1);
				rot.mult(-1);
				speTarget->local.setRot(rot);
			}
			forward.mult(-1);// invert the forward to position the cam on the back, a bit up
			Math::Vector3	pos = speAnchor->local.getPos();
			pos += forward;
			speTarget->local.setPos(pos);
		}
	}
	bool	isCompatible(BehaviorManaged* target) const {
		//dynamic_cast check for Cam
		(void)target;
		return (true);
	}

	void			setAnchor(Object* anchor) {
		this->_anchor = anchor;
	}

	bool			copyRotation;
private:
	Object* _anchor;
	Math::Vector3	_offset;

};

void	scene1() {
	Glfw	glfw(1600, 900);
	glfw.setTitle("This title is long, long enough to test how glfw manages oversized titles. At this point I dont really know what to write, so let's just bullshiting it ....................................................... is that enough? Well, it depends of the size of the current window. I dont really know how many characters i have to write for a width of 1920. Is it possible to higher the police ? It could save some characters. Ok, im bored, lets check if this title is long enough!");

	//Program for Obj3d (can render Obj3d) with vertex & fragmetns shaders
	Obj3dPG			obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);

#ifndef BLUEPRINTS
	//Create Obj3dBP from .obj files
	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = true;

	Obj3dBP			the42BP("obj3d/42.obj");
	Obj3dBP			cubeBP("obj3d/cube.obj");
	Obj3dBP			teapotBP("obj3d/teapot2.obj");
	// Obj3dBP			rocketBP("obj3d/Rocket_Phoenix/AIM-54_Phoenix_OBJ/Aim-54_Phoenix.obj");
	Obj3dBP			rocketBP("obj3d/ARSENAL_VG33/Arsenal_VG33.obj");
	// Obj3dBP			lamboBP("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador.obj");
	Obj3dBP			lamboBP("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider.obj");
	std::cout << "======\n";
#endif
#ifndef TEXTURES
	Texture* texture1 = new Texture("images/lena.bmp");
	Texture* texture2 = new Texture("images/skybox2.bmp");
	Texture* texture3 = new Texture("images/skyboxfuck.bmp");
	//	Texture*	texture4 = new Texture("images/skybox4096.bmp");
	Texture* texture5 = new Texture("images/skytest.bmp");
	// Texture*	texture6 = new Texture("obj3d/Rocket_Phoenix/AIM-54_Phoenix_OBJ/Phoenix.bmp");
	// Texture*	texture6 = new Texture("obj3d/ARSENAL_VG33/Arsenal_VG33.bmp");
//	Texture*	texture7 = new Texture("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborginhi_Aventador_diffuse.bmp");
	Texture		texture8 = *texture1;
#endif
#ifndef OBJ3D
	list<Obj3d*>	obj3dList;

	float s = 1.0f;//scale
	//Create Obj3d with the blueprint & by copy
	Obj3d			the42_1(cubeBP, obj3d_prog);//the42BP !
	the42_1.local.setPos(0, 0, 0);
	the42_1.setTexture(texture1);
	the42_1.displayTexture = true;
	the42_1.setPolygonMode(GL_FILL);
	the42_1.local.setScale(1, 1, 1);

	Obj3d			the42_2(the42_1);
	//the42_2.local.setPos(0, 3, -5);
	the42_2.local.setPos(-4, -2, -2);
	the42_2.setTexture(texture2);
	the42_2.displayTexture = false;
	// the42_2.setScale(3.0f, 0.75f, 0.3f);
	the42_2.setPolygonMode(GL_LINE);

	Obj3d			teapot1(teapotBP, obj3d_prog);
	teapot1.local.setPos(0, 0, 2);
	teapot1.local.getMatrix().setOrder(ROW_MAJOR);
	// teapot1.setRot(0, 90, 0);
	teapot1.setTexture(texture1);
	teapot1.displayTexture = false;
	teapot1.setPolygonMode(GL_LINE);
	// teapot1.setScale(1.5, 2, 0.75);

	Obj3d			cube1(cubeBP, obj3d_prog);
	cube1.local.setPos(0, -2, 3);
	cube1.setTexture(texture1);
	cube1.displayTexture = false;

	Object			empty1;
	empty1.local.setScale(1, 1, 1);

	Obj3d			rocket1(rocketBP, obj3d_prog);
	// rocket1.local.setPos(-10, -20, -2000);
	rocket1.local.setPos(0, -300, 0);
	rocket1.local.rotate(0, 180, 0);
	rocket1.setTexture(texture5);
	rocket1.displayTexture = true;
	// rocket1.setPolygonMode(GL_LINE);
	s = 10.0f;
	rocket1.local.setScale(s, s, s);
	rocket1.setParent(&empty1);


	// Properties::defaultSize = 13.0f;
	Obj3d			lambo1(lamboBP, obj3d_prog);
	lambo1.local.setPos(-20, 0, 0);
	// lambo1.local.setScale(1, 1, 1);
	lambo1.local.setPos(0, -5, 7);
	lambo1.setTexture(texture1);
	lambo1.displayTexture = true;
	lambo1.setPolygonMode(GL_LINE);
	s = 0.025f;
	// lambo1.setScale(s, s, s);

	Obj3d			lambo2(lamboBP, obj3d_prog);
	// lambo2.local.setPos(0, -1.9f, 0);
	lambo2.local.setPos(0, -6.0f, 0);
	// lambo2.local.setScale(1, 5, 1);
	lambo2.local.setRot(0, 180.0f, 0);
	lambo2.setTexture(texture1);
	lambo2.displayTexture = true;
	// lambo2.setPolygonMode(GL_LINE);
	s = 0.4f;
	// lambo2.local.setScale(s, s, s);
	// lambo2.setParent(&the42_1);
	lambo2.setParent(&rocket1);

	Obj3d			lambo3(lamboBP, obj3d_prog);
	lambo3.local.setPos(0, -4, 0);
	lambo3.local.setRot(0, 0.0f, 180);
	lambo3.setTexture(&texture8);
	lambo3.displayTexture = true;
	// lambo3.setPolygonMode(GL_LINE);
	s = 30.0f;
	// lambo3.local.setScale(s, s, s);
	// lambo3.setParent(&the42_1);
	lambo3.setParent(&lambo2);

	obj3dList.push_back(&the42_1);
	// obj3dList.push_back(&the42_2);
	obj3dList.push_back(&teapot1);
	// obj3dList.push_back(&cube1);
	obj3dList.push_back(&rocket1);
	obj3dList.push_back(&lambo1);
	obj3dList.push_back(&lambo2);
	obj3dList.push_back(&lambo3);

	if (true) {//spiral
		// Obj3d*	backObj = &lambo3;
		for (int i = 0; i < 20; i++) {
			Obj3d* lamboPlus = new Obj3d(lamboBP, obj3d_prog);
			lamboPlus->setParent(&lambo3);
			lamboPlus->displayTexture = (i % 2) ? true : false;
			lamboPlus->setTexture(&texture8);
			lamboPlus->setColor(i * 45 % 255, i * 10 % 255, i * 73 % 255);
			float maxScale = 3;
			float scale = (float)((i % (int)maxScale) - (maxScale / 2));
			lamboPlus->local.enlarge(scale, scale, scale);
			float	val = cosf(Math::toRadian(i * 10)) * 10;
			float	coef = 1.0f;
			lamboPlus->local.setPos(lambo3.local.getPos());
			lamboPlus->local.translate(float(i) / coef, val / coef, val / coef);
			lamboPlus->local.rotate(Math::Rotation(i * 5, i * 5, i * 5));

			obj3dList.push_back(lamboPlus);
			// backObj = lamboPlus;
		}
	}
#endif


	// Properties::defaultSize = PP_DEFAULT_SIZE;

	std::cout << &rocket1 << "\n";
	// test_behaviors(rocket1);

	std::cout << "Object # : " << Object::getInstanceAmount() << "\n";
	std::cout << "Obj3d # : " << Obj3d::getInstanceAmount() << "\n";
	std::cout << "\n";
	std::cout << "GL_MAX_CUBE_MAP_TEXTURE_SIZE " << GL_MAX_CUBE_MAP_TEXTURE_SIZE << "\n";
	std::cout << "GL_MAX_TEXTURE_SIZE " << GL_MAX_TEXTURE_SIZE << "\n";

	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);
	Skybox		skybox(*texture3, sky_pg);

#ifndef CAM
	Cam		cam(glfw.getWidth(), glfw.getHeight());
	cam.local.setPos(0, 0, 10);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;

	glfw.setMouseAngle(45);
	std::cout << "MouseAngle: " << glfw.getMouseAngle() << "\n";
	//exit(0);

	if (false) {//cam anchor to rocket1, bugged with Z rot
		cam.local.setPos(0, 1.5f, 3.5f);
		cam.setParent(&rocket1);
		cam.lockedMovement = true;
		cam.lockedOrientation = true;
	}
#endif

	Fps	fps144(144);
	Fps	fps60(60);
	Fps	fps30(30);
	Fps* defaultFps = &fps144;

	//followObjectArgs	st = { defaultFps, &cam };


#ifndef BEHAVIORS
	std::cout << "behavior:\n";
	TransformBH		b1;
	b1.transform.rot.setUnit(ROT_DEG);
	b1.transform.rot.z = 10 * defaultFps->getTick();
	b1.modeRot = ADDITIVE;
	float ss = 1.0f + 0.1f * defaultFps->getTick();
	//b1.transform.scale = Math::Vector3(ss, ss, ss);
//	b1.modeScale = MULTIPLICATIVE;
	std::cout << "___ adding rocket1: " << &rocket1 << "\n";
	b1.addTarget(&rocket1);
	b1.addTarget(&lambo1);
	b1.removeTarget(&lambo1);
	b1.setTargetStatus(&rocket1, false);
	std::cout << "rocket1 status: " << b1.getTargetStatus(&rocket1) << "\n";
	b1.setTargetStatus(&rocket1, true);
	std::cout << "rocket1 status: " << b1.getTargetStatus(&rocket1) << "\n";

	TransformBH		b2;// = b1;//bug
	b2.transform.scale = Math::Vector3(0, 0, 0);
	b2.modeScale = ADDITIVE;
	// b2.transform.rot.z = 0.0f;
	b2.transform.rot.x = -45.0f * defaultFps->getTick();
	// b2.removeTarget(&rocket1);
	b2.addTarget(&empty1);
	//bug if i do:
	b2.addTarget(&empty1);
	b2.removeTarget(&rocket1);

	TransformBH		b3;// = b1;//bug
	b3.transform.scale = Math::Vector3(0, 0, 0);
	b3.modeScale = ADDITIVE;
	b3.transform.rot.y = 102.0f * defaultFps->getTick();
	b3.addTarget(&lambo2);

	std::cout << "b1: " << b1.getTargetList().size() << "\n";
	std::cout << "b2: " << b2.getTargetList().size() << "\n";

	TransformBH		b4;// = b1;//bug
	b4.transform.scale = Math::Vector3(0, 0, 0);
	b4.modeScale = ADDITIVE;
	b4.transform.rot.y = 720.0f * defaultFps->getTick();
	b4.addTarget(&teapot1);
	// exit(0);

	if (false) {// check behavior target, add remove

		std::cout << "behaviorsActive: " << (empty1.behaviorsActive ? "true" : "false") << "\n";
		std::cout << "------------\n";
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";
		std::cout << "b2.addTarget(&lambo1);\n";
		b2.addTarget(&lambo1);
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";
		std::cout << "b2.removeTarget(&lambo1);\n";
		b2.removeTarget(&lambo1);
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";

		std::cout << "------------\n";
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";
		std::cout << "lambo1.addBehavior(&b2);\n";
		lambo1.addBehavior(&b2);
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";
		std::cout << "lambo1.removeBehavior(&b2);\n";
		lambo1.removeBehavior(&b2);
		std::cout << "lambo1:\t" << lambo1.behaviorList.size() << "\n";
		std::cout << "b2:    \t" << b2.getTargetList().size() << "\n";
	}
#endif
	// exit(0);

#ifndef GAMEMANAGER
	GameManager	manager;
	manager.glfw = &glfw;
	manager.cam = &cam;

	glfw.activateDefaultCallbacks(&manager);

#endif

#ifndef RENDER
	std::cout << "Begin while loop\n";
	// cam.local.setScale(s,s,s);//bad, undefined behavior
	while (!glfwWindowShouldClose(glfw._window)) {
		if (defaultFps->wait_for_next_frame()) {
			//////////////////////////////////////////
			if (true) {
				if (false) {
					Math::Matrix4	matRocket = rocket1.getWorldMatrix();
					matRocket.printData();
					std::cout << "---------------\n";
				}
				if (false) {
					the42_1.getWorldMatrix().printData();
				}
				if (false) {
					std::cout << "---rocket1\n" << rocket1.local.getScale().toString() << "\n";
					// rocket1.getWorldMatrix().printData();
					std::cout << "---lambo2\n" << lambo2.local.getScale().toString() << "\n";
					// lambo2.getWorldMatrix().printData();
					std::cout << "---lambo3\n" << lambo3.local.getScale().toString() << "\n";
					// lambo3.getWorldMatrix().printData();
					std::cout << "---------------\n" << lamboBP.getDimensions().toString() << "\n";
					std::cout << "---------------\n";
				}
			}
			////////////////////////////////////////// motion/behaviors
			//this should be used in another func, life a special func managing all events/behavior at every frames
			if (true) {
				b1.run();
				b2.run();
				b3.run();
				b4.run();
				if (false) {
					Math::Vector3 p = teapot1.local.getPos();
					float r = 180 * defaultFps->getTick();
					p.rotateAround(cam.local.getPos(), Math::Rotation(r, 0, 0));
					teapot1.local.setPos(p);
				}
				else if (true) {
					float r = 180 * defaultFps->getTick();
					teapot1.local.rotateAround(cam.local.getPos(), Math::Rotation(r, 0, 0));
				}
				// Math::Matrix4	matEmpty = empty1.getWorldMatrix();
				// matEmpty.printData();
			}
			////////////////////////////////////////// motion end

			glfwPollEvents();
			glfw.updateMouse();//to do before cam's events
			cam.events(glfw, float(defaultFps->getTick()));
			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//renderObj3d(obj3dList, cam);
			//renderSkybox(skybox, cam);
			glfwSwapBuffers(glfw._window);

			if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(glfw._window, GLFW_TRUE);
		}
	}
	std::cout << "End while loop\n";
#endif

	std::cout << "deleting textures...\n";
	delete texture1;
	delete texture2;
	delete texture3;
	//	delete texture4;
	delete texture5;
	//	delete texture6;
		//delete texture7;
}

void scene2() {
	Glfw	glfw(1600, 900);
	glfw.setTitle("Tests camera anchor");

	//Program for Obj3d (can render Obj3d) with vertex & fragmetns shaders
	Obj3dPG			obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG		sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = true;
	Obj3dBP			rocketBP("obj3d/ARSENAL_VG33/Arsenal_VG33.obj");

	Texture* texture3 = new Texture("images/skyboxfuck.bmp");
	Texture* texture5 = new Texture("images/skytest.bmp");

	Skybox		skybox(*texture3, sky_pg);


	list<Obj3d*>	obj3dList;

	Obj3d			rocket(rocketBP, obj3d_prog);
	rocket.local.setPos(0, 0, 0);
	rocket.setTexture(texture5);
	rocket.displayTexture = true;
	float s = 10.0f;
	rocket.local.setScale(s, s, s);

	Obj3d			rocket2(rocketBP, obj3d_prog);
	rocket2.local.setPos(5, 0, 0);
	rocket2.setTexture(texture5);
	rocket2.displayTexture = true;
	s = 5000.0f;
	rocket2.local.setScale(s, s, s);

	obj3dList.push_back(&rocket);
	obj3dList.push_back(&rocket2);


	Cam		cam(glfw.getWidth(), glfw.getHeight());
	cam.local.setPos(0, 15, 35);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;

	glfw.setMouseAngle(-1);
	std::cout << "MouseAngle: " << glfw.getMouseAngle() << "\n";
	//exit(0);

	//cam.local.setPos(0, 1.5f, 3.5f);
	AnchorCameraBH	b1;
	b1.setAnchor(&rocket);
	b1.addTarget(&cam);
	b1.copyRotation = true;
	cam.lockedMovement = true;
	cam.lockedOrientation = true;


	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	float	mvt = 30.0f * defaultFps->getTick();

	std::cout << "Begin while loop\n";
	int c = 0;
	while (!glfwWindowShouldClose(glfw._window)) {
		if (defaultFps->wait_for_next_frame()) {

			if (1) {
				b1.run();
			}

			//print data
			if (false) {
				c++;
				if (c == 60) {
					c = 0;
					system("cls");
					if (true) {
						std::cout << "--- ROCKET\n";
						//rocket.local.getScale().toString() << "\n";
						std::cout << rocket.local.getRot().toString() << "\n";
						rocket.getWorldMatrix().printData();
					}
					if (true) {
						std::cout << "--- CAM\n" << cam.local.getRot().toString();
						std::cout << "- local\n";
						cam.getLocalProperties().getMatrix().printData();
						std::cout << "- world\n";
						cam.getWorldMatrix().printData();
					}
				}
			}

			glfwPollEvents();
			glfw.updateMouse();//to do before cam's events
			cam.events(glfw, float(defaultFps->getTick()));
			if (true) {
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_LEFT))
					rocket.local.rotate(0, mvt, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_RIGHT))
					rocket.local.rotate(0, -mvt, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_DOWN))
					rocket.local.rotate(mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_UP))
					rocket.local.rotate(-mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_1))
					rocket.local.rotate(0, 0, mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_2))
					rocket.local.rotate(0, 0, -mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_0))
					rocket.local.setRot(0, 0, 0);

				Math::Vector3	pos = rocket.local.getPos();
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_8))
					pos.add(0, 0, -mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_5))
					pos.add(0, 0, mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_4))
					pos.add(-mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_6))
					pos.add(mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_7))
					pos.add(0, -mvt, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_9))
					pos.add(0, mvt, 0);
				rocket.local.setPos(pos);


				//#define 	GLFW_KEY_RIGHT   262
				//#define 	GLFW_KEY_LEFT   263
				//#define 	GLFW_KEY_DOWN   264
				//#define 	GLFW_KEY_UP   265
			}

			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//renderObj3d(obj3dList, cam);
			//renderSkybox(skybox, cam);
			glfwSwapBuffers(glfw._window);

			if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(glfw._window, GLFW_TRUE);
		}
	}
	std::cout << "End while loop\n";

	std::cout << "deleting textures...\n";
	delete texture3;
	delete texture5;
}

#define BORDERS_ON	true
#define BORDERS_OFF	false
void	fillData(uint8_t* dst, QuadNode* node, int* leafAmount, int baseWidth, bool draw_borders, int threshold, Math::Vector3 color) {
	if (!node)
		return;
	//if (node->isLeaf()) {
	if (node->detail <= threshold) {
		//(*leafAmount)++;
		//std::cout << "leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\n";
		if (node->width == 0 || node->height == 0) {
			std::cout << "error with tree data\n"; std::exit(2);
		}
		if (node->width * node->height >= DEBUG_LEAF_AREA && DEBUG_LEAF && *leafAmount == 0 && DEBUG_FILL_TOO) {
			std::cout << "Fill new leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\t";
			std::cout << (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << "\n";
		}
		for (int j = 0; j < node->height; j++) {
			for (int i = 0; i < node->width; i++) {
				unsigned int posx = node->x + i;
				unsigned int posy = node->y + j;
				unsigned int index = (posy * baseWidth + posx) * 3;
				//std::cout << ((posy * baseWidth + posx) * 3 + 0) << "\n";
				//std::cout << ((posy * baseWidth + posx) * 3 + 1) << "\n";
				//std::cout << ((posy * baseWidth + posx) * 3 + 2) << "\n";
				if (draw_borders && (i == 0 || j == 0)) {
					dst[index + 0] = 0;
					dst[index + 1] = 0;
					dst[index + 2] = 0;
				}
				else {
					if (1 && (posx == 0 || posy == 0)) {
						dst[index + 0] = color.x;
						dst[index + 1] = color.y;
						dst[index + 2] = color.z;
					}
					else {
						dst[index + 0] = node->pixel.r;
						dst[index + 1] = node->pixel.g;
						dst[index + 2] = node->pixel.b;
					}
				}
			}
		}
	}
	else if (node->children) {
		fillData(dst, node->children[0], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[1], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[2], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[3], leafAmount, baseWidth, draw_borders, threshold, color);
	}
}
#define THRESHOLD 0
QuadNode* textureToQuadTree(Texture* tex) {
	uint8_t* data = tex->getData();
	unsigned int	w = tex->getWidth();
	unsigned int	h = tex->getHeight();
	Pixel** pix = new Pixel * [h];
	for (size_t j = 0; j < h; j++) {
		pix[j] = new Pixel[w];
		for (size_t i = 0; i < w; i++) {
			pix[j][i].r = data[(j * w + i) * 3 + 0];
			pix[j][i].g = data[(j * w + i) * 3 + 1];
			pix[j][i].b = data[(j * w + i) * 3 + 2];
		}
	}
	std::cout << "pixel: " << sizeof(Pixel) << "\n";

	QuadNode* root = new QuadNode(pix, 0, 0, w, h, THRESHOLD);
	std::cout << "root is leaf: " << (root->isLeaf() ? "true" : "false") << "\n";

	return root;
}

class QuadTreeManager : public GameManager {
public:
	QuadTreeManager() : GameManager() {
		this->threshold = 0;
		this->draw_borders = BORDERS_OFF;
	}
	virtual ~QuadTreeManager() {}
	unsigned int	threshold;
	bool			draw_borders;
};

static void		keyCallback_quadTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << "\n";

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS\n";
		QuadTreeManager* manager = static_cast<QuadTreeManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			std::cout << "static_cast failed\n";
		}
		else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER)
				manager->draw_borders = !manager->draw_borders;
		}
	}
}

void	scene_4Tree() {
	QuadTreeManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->setTitle("Tests texture quadtree");
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_quadTree;

	Texture* lena = new Texture(WIN32_VS_FOLDER + "images/lena.bmp");
	Texture* rgbtest = new Texture(WIN32_VS_FOLDER + "images/test_rgb.bmp");
	Texture* minirgb = new Texture(WIN32_VS_FOLDER + "images/minirgb.bmp");
	Texture* red = new Texture(WIN32_VS_FOLDER + "images/red.bmp");
	Texture* monkey = new Texture(WIN32_VS_FOLDER + "images/monkey.bmp");
	Texture* flower = new Texture(WIN32_VS_FOLDER + "images/flower.bmp");

	Texture* baseImage = flower;
	int w = baseImage->getWidth();
	int h = baseImage->getHeight();
	QuadNode* root = new QuadNode(baseImage->getData(), w, 0, 0, w, h, THRESHOLD);
	uint8_t* dataOctree = new uint8_t[w * h * 3];

	float size_coef = float(WINX) / float(baseImage->getWidth()) / 2.0f;
	UIImage	uiBaseImage(baseImage);
	uiBaseImage.setPos(0, 0);
	uiBaseImage.setSize(uiBaseImage.getTexture()->getWidth() * size_coef, uiBaseImage.getTexture()->getHeight() * size_coef);

	Texture* image4Tree = new Texture(dataOctree, w, h);

	UIImage	ui4Tree(image4Tree);
	ui4Tree.setPos(baseImage->getWidth() * size_coef, 0);
	ui4Tree.setSize(ui4Tree.getTexture()->getWidth() * size_coef, ui4Tree.getTexture()->getHeight() * size_coef);

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	std::cout << "Begin while loop\n";
	int	leafAmount = 0;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			fillData(dataOctree, root, &leafAmount, w, manager.draw_borders, manager.threshold, Math::Vector3(0, 0, 0));
			leafAmount = -1;
			//std::cout << "leafs: " << leafAmount << "\n";
			//std::cout << "w * h * 3 = " << w << " * " << h << " * 3 = " << w * h * 3 << "\n";
			image4Tree->updateData(dataOctree, root->width, root->height);

			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiBaseImage);
			//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &ui4Tree);
			glfwSwapBuffers(manager.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "End while loop\n";
	std::cout << "deleting textures...\n";
	delete lena;
	delete red;
	delete rgbtest;
}

class ProceduralManager : public GameManager {
public:
	ProceduralManager() : GameManager() {
		this->perlin = nullptr;
		this->core_amount = std::thread::hardware_concurrency();
		std::cout << " number of cores: " << this->core_amount << "\n";
		this->seed = 888;
		std::srand(this->seed);
		this->frequency = 4;
		this->frequency = std::clamp(this->frequency, 0.1, 64.0);
		this->octaves = 12;
		this->octaves = std::clamp(this->octaves, 1, 16);
		this->flattering = 1;
		this->flattering = std::clamp(this->flattering, 0.01, 10.0);
		this->posOffsetX = 0;
		this->posOffsetY = 0;
		//tmp
		this->mouseX = 0;
		this->mouseY = 0;
		this->areaWidth = 0;
		this->areaHeight = 0;
		this->island = 0;
		this->island = std::clamp(this->island, -2.0, 2.0);
		//vox
		this->player = nullptr;
		this->playerChunkX = 0;
		this->playerChunkY = 0;
		this->range_chunk_display = 5;
		this->range_chunk_memory = 41;
		this->chunk_size = 32;
		this->voxel_size = 1;
		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 5;
	}
	/*
		sans opti:
		30*30*256 = 775 680 octets / chunk
		775 680 * 9 = 6 981 120 octets (block de jeu) affichés
		6 981 120 * 6 * 2 = 83 773 440 polygones
		775 680 * 49 = 38 008 320 octets en memoire
	*/

	virtual ~ProceduralManager() {}
	siv::PerlinNoise* perlin;
	unsigned int	core_amount;
	unsigned int	seed;
	double			frequency;
	int				octaves;
	double			flattering;
	int				posOffsetX;
	int				posOffsetY;
	//tmp
	double			mouseX;
	double			mouseY;
	int				areaWidth;
	int				areaHeight;
	double			island;
	//vox
	Obj3d* player;
	int	playerChunkX;
	int	playerChunkY;
	std::list<Obj3d*>	renderlist;
	Cam* cam;
	int				range_chunk_display;
	int				range_chunk_memory;
	int				chunk_size;
	int				voxel_size;
	GLuint			polygon_mode;
	int				threshold;
};

void TestPerlin()
{
	siv::PerlinNoise perlinA(std::random_device{});
	siv::PerlinNoise perlinB;

	std::array<std::uint8_t, 256> state;
	perlinA.serialize(state);
	perlinB.deserialize(state);

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(1234);
	perlinB.reseed(1234);

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(std::mt19937{ 1234 });
	perlinB.reseed(std::mt19937{ 1234 });

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));
}

Math::Vector3	genColor(uint8_t elevation) {
	double value = double(elevation) / 255.0;
	Math::Vector3	color;

	if (elevation < 50) { // water
		color.x = 0;
		color.y = uint8_t(150.0 * std::clamp((double(elevation) / 50.0), 0.25, 1.0));
		color.z = uint8_t(255.0 * std::clamp((double(elevation) / 50.0), 0.25, 1.0));
	}
	else if (elevation < 75) { // sand
		color.x = 255.0 * ((double(elevation)) / 75.0);
		color.y = 200.0 * ((double(elevation)) / 75.0);
		color.z = 100.0 * ((double(elevation)) / 75.0);
	}
	else if (elevation > 200) { // snow
		color.x = elevation;
		color.y = elevation;
		color.z = elevation;
	}
	else if (elevation > 175) { // rocks
		color.x = 150.0 * value;
		color.y = 150.0 * value;
		color.z = 150.0 * value;
	}
	else {//grass
		color.x = 0;
		color.y = 200.0 * value;
		color.z = 100.0 * value;

	}
	return color;
}

void	th_buildData(uint8_t* data, ProceduralManager& manager, int yStart, int yEnd) {
	const siv::PerlinNoise perlin(manager.seed);
	double playerPosX, playerPosY;
	playerPosX = manager.mouseX - (WINX / 2);//center of screen is 0:0
	playerPosY = WINY - manager.mouseY - (WINY / 2);//center of screen is 0:0   //invert glfw Y to match opengl image
	playerPosX += manager.posOffsetX;
	playerPosY += manager.posOffsetY;
	//std::cout << playerPosX << " : " << playerPosY << "\n";
	int screenCornerX, screenCornerY;
	screenCornerX = playerPosX - (manager.areaWidth / 2);
	screenCornerY = playerPosY - (manager.areaHeight / 2);


	for (int y = yStart; y < yEnd; ++y) {
		for (int x = 0; x < manager.areaWidth; ++x) {
			double value;
			double posX = screenCornerX + x;//pos of the generated pixel/elevation/data
			double posY = screenCornerY + y;
			double nx = double(posX) / double(manager.areaWidth);//normalised 0..1
			double ny = double(posY) / double(manager.areaHeight);

			value = perlin.accumulatedOctaveNoise2D_0_1(nx * manager.frequency,
				ny * manager.frequency,
				manager.octaves);
			value = std::pow(value, manager.flattering);
			Math::Vector3	vec(posX, posY, 0);
			double dist = (double(vec.len()) / double(WINY / 2));//normalized 0..1
			value = std::clamp(value + manager.island * (0.5 - dist), 0.0, 1.0);

			int index = (y * manager.areaWidth + x) * 3;
			uint8_t color = (uint8_t)(value * 255.0);
			if (color < 50) { // water
				data[index + 0] = 0;
				data[index + 1] = uint8_t(150.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
				data[index + 2] = uint8_t(255.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
			}
			else if (color < 75) { // sand
				data[index + 0] = 255.0 * ((double(color)) / 75.0);
				data[index + 1] = 200.0 * ((double(color)) / 75.0);
				data[index + 2] = 100.0 * ((double(color)) / 75.0);
			}
			else if (color > 200) { // snow
				data[index + 0] = color;
				data[index + 1] = color;
				data[index + 2] = color;
			}
			else if (color > 175) { // rocks
				data[index + 0] = 150.0 * value;
				data[index + 1] = 150.0 * value;
				data[index + 2] = 150.0 * value;
			}
			else {//grass
				data[index + 0] = 0;
				data[index + 1] = 200.0 * value;
				data[index + 2] = 100.0 * value;

			}
		}
	}
}

void	scene_procedural() {
	TestPerlin();
	ProceduralManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->toggleCursor();
	manager.glfw->setTitle("Tests procedural");
	manager.glfw->activateDefaultCallbacks(&manager);

	const siv::PerlinNoise perlin(manager.seed);

	int screenSize = 800;
	manager.areaWidth = screenSize;
	manager.areaHeight = screenSize;
	manager.frequency = double(screenSize) / 75;
	uint8_t* data = new uint8_t[manager.areaWidth * manager.areaHeight * 3];
	Texture* image = new Texture(data, manager.areaWidth, manager.areaHeight);

	int repeatX = WINX / manager.areaWidth;
	int repeatY = WINY / manager.areaHeight;
	std::cout << "repeatX: " << repeatX << "\n";
	std::cout << "repeatZ: " << repeatY << "\n";
	float size_coef = float(WINX) / float(image->getWidth()) / float(repeatX < repeatY ? repeatX : repeatY);
	size_coef = 1;
	UIImage	uiImage(image);
	uiImage.setPos(0, 0);
	uiImage.setSize(uiImage.getTexture()->getWidth() * size_coef, uiImage.getTexture()->getHeight() * size_coef);


	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	int thread_amount = manager.core_amount - 1;
	std::thread* threads_list = new std::thread[thread_amount];

	std::cout << "Begin while loop\n";
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			// printFps();

			glfwGetCursorPos(manager.glfw->_window, &manager.mouseX, &manager.mouseY);
			int playerPosX = manager.mouseX - (WINX / 2);//center of screen is 0:0
			int playerPosY = WINY - manager.mouseY - (WINY / 2);//center of screen is 0:0   //invert glfw Y to match opengl image
			Math::Vector3	vec(playerPosX, playerPosY, 0);
			double dist = (double(vec.len()) / double(WINY * 2));
			std::cout << playerPosX << ":" << playerPosY << "  \t" << dist << "\n";

			for (size_t i = 0; i < thread_amount; i++) {//compute data with threads
				int start = ((manager.areaHeight * (i + 0)) / thread_amount);
				int end = ((manager.areaHeight * (i + 1)) / thread_amount);
				//std::cout << start << "\t->\t" << end << "\t" << end - start << "\n";
				threads_list[i] = std::thread(th_buildData, std::ref(data), std::ref(manager), start, end);
			}
			for (size_t i = 0; i < thread_amount; i++) {
				threads_list[i].join();
			}

			image->updateData(data, manager.areaWidth, manager.areaHeight);
			uiImage.setPos(manager.mouseX - (manager.areaWidth / 2), WINY - manager.mouseY - (manager.areaHeight / 2));
			uiImage.setSize(uiImage.getTexture()->getWidth() * size_coef, uiImage.getTexture()->getHeight() * size_coef);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiImage);
			glfwSwapBuffers(manager.glfw->_window);


			int mvtSpeed = 5;
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_UP)) {
				manager.posOffsetY += mvtSpeed;
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_DOWN)) {
				manager.posOffsetY -= mvtSpeed;
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_RIGHT)) {
				manager.posOffsetX += mvtSpeed;
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_LEFT)) {
				manager.posOffsetX -= mvtSpeed;
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_7)) {
				manager.frequency += 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_4)) {
				manager.frequency -= 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_8)) {
				manager.flattering += 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_5)) {
				manager.flattering -= 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_9)) {
				manager.island += 0.05;
				manager.island = std::clamp(manager.island, 0.01, 2.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_6)) {
				manager.island -= 0.05;
				manager.island = std::clamp(manager.island, -2.0, 2.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
		}
	}
	delete[] threads_list;

	std::cout << "End while loop\n";
	std::cout << "deleting textures...\n";
}

void	buildChunk(ProceduralManager& manager, QuadNode* node, int chunkI, int chunkJ, int threshold) {//can be used for every tree node
	if (!node)
		return;
	//if (node->isLeaf()) {
	if (node->detail <= threshold) {
		//std::cout << "leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\n";
		if (node->width == 0 || node->height == 0) {
			std::cout << "error with tree data\n"; std::exit(2);
		}
		if (node->width * node->height >= DEBUG_LEAF_AREA && DEBUG_LEAF && DEBUG_BUILD_TOO) {
			std::cout << "Display new leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\t";
			std::cout << (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << "\n";
		}
#ifndef RENDER_WORLD // make a special render func for that (or change buildChunk)
		Obj3d* cube = manager.renderlist.front();
		int polmode = cube->getPolygonMode();
		uint8_t	elevation = node->pixel.r;
		Math::Vector3	color = genColor(elevation);
		if (0 && (node->x == 0 || node->y == 0))
			cube->setColor(0, 0, 0);
		else
			cube->setColor(color.x, color.y, color.z);

		int worldPosX = manager.playerChunkX * manager.chunk_size + node->x + chunkI * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size);
		int worldPosY = manager.playerChunkY * manager.chunk_size + node->y + chunkJ * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size);

		cube->local.setPos(worldPosX, 0, worldPosY);
		cube->local.setScale(node->width, node->pixel.r / 3, node->height);// height is opengl z
		//cube->local.setScale(node->width, 1, node->height);// height is opengl z

		cube->setPolygonMode(manager.polygon_mode);
		//renderObj3d(manager.renderlist, *manager.cam);
		cube->setPolygonMode(polmode);
#endif
	}
	else if (node->children) {
		buildChunk(manager, node->children[0], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[1], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[2], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[3], chunkI, chunkJ, threshold);
	}
}

void	buildWorld(ProceduralManager& manager, QuadNode*** memory4Tree) {
	//std::cout << "building world...\n";

	int longTreshold = 12;
	int midTreshold = 8;
	int threshold = 3;

	int startMid = (manager.range_chunk_memory / 2) - (20 / 2);
	int endMid = startMid + 20;
	int start = (manager.range_chunk_memory / 2) - (manager.range_chunk_display / 2);
	int end = start + manager.range_chunk_display;
	std::cout << start << " -> " << end << "\t/ " << manager.range_chunk_memory << "\n";
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << j << ":" << i << "\n";
			if (i >= start && i < end && j >= start && j < end)//around player
				threshold = manager.threshold;
			else if (i >= startMid && i < endMid && j >= startMid && j < endMid)//around player
				threshold = midTreshold;
			buildChunk(manager, memory4Tree[j][i], i, j, threshold);
			threshold = longTreshold;
		}
	}
}

uint8_t* generatePerlinNoise(ProceduralManager& manager, int posX, int posY, int width, int height) {
	uint8_t* data = new uint8_t[width * height * 3];
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double value;
			double nx = double(posX + x) / double(500);//normalised 0..1
			double ny = double(posY + y) / double(500);

			value = manager.perlin->accumulatedOctaveNoise2D_0_1(nx * manager.frequency,
				ny * manager.frequency,
				manager.octaves);

			value = std::pow(value, manager.flattering);
			Math::Vector3	vec(x, y, 0);
			double dist = (double(vec.len()) / double(WINY / 2));//normalized 0..1
			value = std::clamp(value + manager.island * (0.5 - dist), 0.0, 1.0);

			uint8_t color = (uint8_t)(value * 255.0);
			int index = (y * width + x) * 3;
			data[index + 0] = color;
			data[index + 1] = color;
			data[index + 2] = color;
		}
	}
	return data;
}

void	updateChunksX(ProceduralManager& manager, QuadNode*** chunkMemory4Tree, uint8_t*** chunkMemory, int change) {
	if (abs(change) > 1) {
		std::cout << "player went too fast on X, or teleported\n";
		std::exit(1);
	}

	bool displayDebug = false;

	manager.playerChunkX += change;
	int startX = manager.playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = manager.playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);

	if (change > 0) {//if we went on right, we shift everything on left, and rebuild last column
		for (int i = 0; i < manager.range_chunk_memory; i++) {
			for (int j = 0; j < manager.range_chunk_memory; j++) {
				if (i == 0) {//delete first column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first column on line " << j << " column " << i << "\n";
				}
				if (i == manager.range_chunk_memory - 1) {//last column, rebuild
					if (displayDebug)
						std::cout << "rebuild last column on line " << j << " column " << i << "\n";
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
				}
				else {//shift on left
					if (displayDebug)
						std::cout << "shift on left on line " << j << " column " << i << "\n";
					chunkMemory[j][i] = chunkMemory[j][i + 1];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j][i + 1];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	else if (change < 0) {//if we went on left, ie shift everything on right, rebuild first column
		for (int i = manager.range_chunk_memory - 1; i >= 0; i--) {
			for (int j = 0; j < manager.range_chunk_memory; j++) {
				if (i == manager.range_chunk_memory - 1) {//delete last column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete last column on line " << j << " column " << i << "\n";
				}
				if (i == 0) {//first column, rebuild
					if (displayDebug)
						std::cout << "rebuild first column on line " << j << " column " << i << "\n";
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
				}
				else {//shift on right
					if (displayDebug)
						std::cout << "shift on right on line " << j << " column " << i << "\n";
					chunkMemory[j][i] = chunkMemory[j][i - 1];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j][i - 1];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	std::cout << "updated Chunks X\n";
}

void	updateChunksY(ProceduralManager& manager, QuadNode*** chunkMemory4Tree, uint8_t*** chunkMemory, int change) {
	if (abs(change) > 1) {
		std::cout << "player went too fast on Y, or teleported\n";
		std::exit(1);
	}

	bool displayDebug = false;

	manager.playerChunkY += change;
	int startX = manager.playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = manager.playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);

	if (change > 0) {//if we went on bottom, we shift everything on top, and rebuild last column
		for (int j = 0; j < manager.range_chunk_memory; j++) {
			for (int i = 0; i < manager.range_chunk_memory; i++) {
				if (j == 0) {//delete first line
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first line on column " << i << " line " << j << "\n";
				}
				if (j == manager.range_chunk_memory - 1) {//last line, rebuild
					if (displayDebug)
						std::cout << "rebuild last line on column " << i << " line " << j << "\n";
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
				}
				else {//shift on top
					if (displayDebug)
						std::cout << "shift on top on column " << i << " line " << j << "\n";
					chunkMemory[j][i] = chunkMemory[j + 1][i];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j + 1][i];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	else if (change < 0) {//if we went on left, ie shift everything on right, rebuild first column
		for (int j = manager.range_chunk_memory - 1; j >= 0; j--) {
			for (int i = 0; i < manager.range_chunk_memory; i++) {
				if (j == manager.range_chunk_memory - 1) {//delete last column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first line on column " << i << " line " << j << "\n";
				}
				if (j == 0) {//first column, rebuild
					if (displayDebug)
						std::cout << "rebuild last line on column " << i << " line " << j << "\n";
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
				}
				else {//shift on bottom
					if (displayDebug)
						std::cout << "shift on top on column " << i << " line " << j << "\n";
					chunkMemory[j][i] = chunkMemory[j - 1][i];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j - 1][i];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	std::cout << "updated Chunks Y\n";
}

static void		keyCallback_vox(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << "\n";

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS\n";
		ProceduralManager* manager = static_cast<ProceduralManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			std::cout << "static_cast failed\n";
		}
		else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER) {
				manager->polygon_mode++;
				manager->polygon_mode = GL_POINT + (manager->polygon_mode % 3);
			}
		}
	}
}

void	scene_vox() {
#ifndef INIT_GLFW
	Obj3dBP::defaultSize = 1;
	TestPerlin();
	ProceduralManager	manager;
	siv::PerlinNoise perlin(manager.seed);
	manager.perlin = &perlin;
	manager.glfw = new Glfw(WINX, WINY);
	//glDisable(GL_CULL_FACE);
	manager.glfw->setTitle("Tests vox");
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_vox;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_vox;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_vox;

	Obj3dPG		obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP		cubebp("obj3d/cube.obj");

	Texture* tex_skybox = new Texture("images/skybox4.bmp");
	Skybox		skybox(*tex_skybox, sky_pg);

	Cam		cam(manager.glfw->getWidth(), manager.glfw->getHeight());
	cam.speed = 4 * 15;
	cam.local.setPos(0, 30, 0);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//manager.glfw->setMouseAngle(-1);//?
	std::cout << "MouseAngle: " << manager.glfw->getMouseAngle() << "\n";
	manager.cam = &cam;

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps60;

#endif// INIT_GLFW

#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, obj3d_prog);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	manager.renderlist.push_back(&cubeo);

	Obj3d		player1(cubebp, obj3d_prog);
	player1.local.setPos(0, 35, 0);
	player1.local.setScale(1, 2, 1);
	player1.local.enlarge(5, 5, 5);
	player1.setColor(255, 0, 0);
	player1.displayTexture = false;
	player1.setPolygonMode(GL_FILL);
	manager.player = &player1;

	std::list<Obj3d*>	playerList;
	playerList.push_back(&player1);
#endif

#ifndef CHUNKS
	// build the heightmaps depending of tile size (for now 7x7 tiles, tile is 30x30 cubes)
	std::cout << "manager.chunk_size " << manager.chunk_size << "\n";
	std::cout << "manager.range_chunk_memory " << manager.range_chunk_memory << "\n";
	std::cout << "manager.range_chunk_display " << manager.range_chunk_display << "\n";

	uint8_t*** chunkMemory = new uint8_t * *[manager.range_chunk_memory];
	QuadNode*** chunkMemory4Tree = new QuadNode * *[manager.range_chunk_memory];
	Math::Vector3	playerPos = player1.local.getPos();
	int	playerChunkX = (int)playerPos.x / manager.chunk_size;
	int	playerChunkY = (int)playerPos.z / manager.chunk_size;//opengl y is height, so we use opengl z here
	int startX = playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		chunkMemory[j] = new uint8_t * [manager.range_chunk_memory];
		chunkMemory4Tree[j] = new QuadNode * [manager.range_chunk_memory];
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << i << ":" << j << "\n";
			chunkMemory[j][i] = nullptr;
			chunkMemory4Tree[j][i] = nullptr;
			int x, y, w, h;
			x = startX + manager.chunk_size * i;
			y = startY + manager.chunk_size * j;
			w = manager.chunk_size;
			h = manager.chunk_size;
			chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);//make a class Rect ?
			chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
		}
	}
	int startDisplay = (manager.range_chunk_memory - manager.range_chunk_display) / 2;
	int endDisplay = startDisplay + manager.range_chunk_display;
	int memoryTotalRange = manager.chunk_size * manager.range_chunk_memory;
	uint8_t* dataChunkMemory = new uint8_t[3 * memoryTotalRange * memoryTotalRange];
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
			int leafamount = 0;
			Math::Vector3 color(0, 0, 0);
			if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
				color = Math::Vector3(255, 0, 0);
			fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
				chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
		}
	}

	Texture* grid = new Texture(dataChunkMemory, memoryTotalRange, memoryTotalRange);
	UIImage		gridPanel(grid);
	gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
		(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
	gridPanel.setSize(memoryTotalRange, memoryTotalRange);
#endif // CHUNKS



	//int thread_amount = manager.core_amount - 1;
	//std::thread* threads_list = new std::thread[thread_amount];

	std::cout << "Begin while loop\n";
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			manager.glfw->updateMouse();//to do before cam's events
			cam.events(*(manager.glfw), float(defaultFps->getTick()));

			//cubeb.local.rotate(50 * defaultFps->getTick(), 50 * defaultFps->getTick(), 0);

			//defaultFps->printFps();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			buildWorld(manager, chunkMemory4Tree);
			//renderObj3d(playerList, cam);
			//renderSkybox(skybox, cam);
			//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &gridPanel);
			glfwSwapBuffers(manager.glfw->_window);


#ifndef KEY_EVENTS
			int mvtSpeed = 5;
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_UP)) {
				manager.posOffsetY += mvtSpeed;
				player1.local.translate(VEC3_FORWARD);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_DOWN)) {
				manager.posOffsetY -= mvtSpeed;
				player1.local.translate(VEC3_BACKWARD);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_RIGHT)) {
				manager.posOffsetX += mvtSpeed;
				player1.local.translate(VEC3_RIGHT);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_LEFT)) {
				manager.posOffsetX -= mvtSpeed;
				player1.local.translate(VEC3_LEFT);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_SPACE)) {
				player1.local.translate(VEC3_UP);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_C)) {
				player1.local.translate(VEC3_DOWN);
			}


			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_7)) {
				manager.frequency += 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_4)) {
				manager.frequency -= 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_8)) {
				manager.flattering += 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_5)) {
				manager.flattering -= 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_9)) {
				manager.island += 0.05;
				manager.island = std::clamp(manager.island, 0.01, 2.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_6)) {
				manager.island -= 0.05;
				manager.island = std::clamp(manager.island, -2.0, 2.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
#endif
			Math::Vector3	playerpos = manager.player->local.getPos();
			int currentchunkX = int(playerpos.x / manager.chunk_size);
			int currentchunkY = int(playerpos.z / manager.chunk_size);//opengl y is height, so we use opengl z here
			if (playerpos.x < 0) // cauz x=-29..+29 ----> x / 30 = 0; 
				currentchunkX--;
			if (playerpos.z < 0) // same
				currentchunkY--;
			if (currentchunkX != manager.playerChunkX) {
				updateChunksX(manager, chunkMemory4Tree, chunkMemory, currentchunkX - manager.playerChunkX);
				std::cout << "===<<< Player changed chunk X: " << manager.playerChunkX << ":" << manager.playerChunkY << "\n";
				std::cout << "filling data to texture... ";
				for (size_t j = 0; j < manager.range_chunk_memory; j++) {
					for (size_t i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
					}
				}
				std::cout << "Done\n";
				gridPanel.getTexture()->updateData(dataChunkMemory, memoryTotalRange, memoryTotalRange);
				playerPos = player1.local.getPos();
				gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
					(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
				gridPanel.setSize(memoryTotalRange, memoryTotalRange);
			}
			if (currentchunkY != manager.playerChunkY) {
				updateChunksY(manager, chunkMemory4Tree, chunkMemory, currentchunkY - manager.playerChunkY);
				std::cout << "===<<< Player changed chunk Y: " << manager.playerChunkX << ":" << manager.playerChunkY << "\n";
				std::cout << "filling data to texture... ";
				for (size_t j = 0; j < manager.range_chunk_memory; j++) {
					for (size_t i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
					}
				}
				std::cout << "Done\n";
				gridPanel.getTexture()->updateData(dataChunkMemory, memoryTotalRange, memoryTotalRange);
				playerPos = player1.local.getPos();
				gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
					(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
				gridPanel.setSize(memoryTotalRange, memoryTotalRange);
			}

		}
	}
	//delete[] threads_list;

	std::cout << "End while loop\n";
	std::cout << "deleting textures...\n";
	delete tex_skybox;
}

class OctreeManager : public QuadTreeManager
{
public:
	OctreeManager() : QuadTreeManager() {
		this->seed = 777;
		std::srand(this->seed);
		this->perlin = new siv::PerlinNoise(this->seed);
		this->ps = new PerlinSettings(*this->perlin);

		this->ps->frequency = 6;
		this->ps->octaves = 6;//def 6
		this->ps->flattering = 0.6;//1 for no impact
		this->ps->island = 0;// 0 for no impact
		this->ps->heightCoef = 0.5;

		//default
		this->ps->flattering = PERLIN_DEF_FLATTERING;
		this->ps->island = PERLIN_DEF_ISLAND;

		this->ps->frequency = std::clamp(this->ps->frequency, 0.1, 64.0);
		this->ps->octaves = std::clamp((int)this->ps->octaves, 1, 16);
		this->ps->flattering = std::clamp(this->ps->flattering, 0.1, 5.0);
		this->ps->island = std::clamp(this->ps->island, 0.1, 5.0);

		this->minimapPanels = nullptr;
		this->playerMinimap = nullptr;
		this->minimapCoef = 0.8f;
		this->player = nullptr;
		this->playerSpeed = 20;//unit/s
		this->shiftPressed = false;
		int s = 32;
		this->chunk_size = Math::Vector3(s, s, s);
		int	g = 5;
		int	d = 3;// g * 2 / 3;
		this->gridSize = Math::Vector3(g, g, g);
		this->gridSizeDisplayed = Math::Vector3(d, d, d);

		this->minimapCenterX = this->chunk_size.x * this->gridSize.x * 0.5f * this->minimapCoef;
		this->minimapCenterZ = this->chunk_size.z * this->gridSize.z * 0.5f * this->minimapCoef;

		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 0;

		this->cpu_coreAmount = std::thread::hardware_concurrency();
	}
	~OctreeManager() {}

	unsigned int		seed;
	siv::PerlinNoise* perlin;
	PerlinSettings* ps;
	std::list<Object*>	renderlist;
	std::list<Object*>	renderlistOctree;
	std::list<Object*>	renderlistGrid;
	std::list<Object*>	renderlistVoxels[6];//6faces
	std::list<Object*>	renderlistChunk;
	std::list<Object*>	renderlistSkybox;
	UIPanel*** minimapPanels;
	UIImage* playerMinimap;
	float				minimapCoef;
	int					minimapCenterX;
	int					minimapCenterZ;
	GLuint				polygon_mode;
	Obj3d* player;
	float				playerSpeed;
	bool				shiftPressed;
	Math::Vector3		chunk_size;
	Math::Vector3		gridSize;
	Math::Vector3		gridSizeDisplayed;
	unsigned int		threshold;

	unsigned int		cpu_coreAmount;
};

void	scene_benchmarks() {
	char input[100];
	std::cout << "Choose:\n\t";
	std::cout << "1 - glDrawArrays\n\t";
	std::cout << "2 - glDrawElements\n\t";
	std::cout << "3 - glDrawArraysInstanced\n\t";
	std::cout << "4 - glDrawElementsInstanced\n";
	input[0] = '1';	input[1] = 0;
	std::cin >> input;
	OctreeManager	m;
	m.glfw = new Glfw(WINX, WINY);

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP::defaultDataMode = BP_INDICES;
	if (input[0] % 2 == 1)
		Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Obj3dBP		lambobp(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider.obj");

	Texture* lenatex = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* lambotex = new Texture(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborginhi_Aventador_diffuse.bmp");
	Texture* tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");

	Obj3dPG		rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	Obj3dIPG	rendererObj3dInstanced(SIMPLEGL_FOLDER + OBJ3D_INSTANCED_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG	rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);

	Skybox		skybox(*tex_skybox, rendererSkybox);
	m.renderlistSkybox.push_back(&skybox);

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.local.setPos(-5, -5, -5);
	cam.speed = 5;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	m.cam = &cam;

	Obj3dPG* renderer = &rendererObj3d;
	if (input[0] >= '3') {
		renderer = &rendererObj3dInstanced;
		std::cout << "Instanced rendering enabled\n";
	}

	//texture edit
	uint8_t* tex_data = lenatex->getData();
	unsigned int w = lenatex->getWidth();
	unsigned int h = lenatex->getHeight();
	int	maxs = w * h * 3;
	for (int i = 0; i < maxs; i++) {
		if (i % 3 == 0)
			tex_data[i] = 255;
	}
	lenatex->updateData(tex_data, w, h);


	if (1) {//cubes
		for (size_t i = 0; i < 10; i++) {
			for (size_t j = 0; j < 10; j++) {
				Obj3d* cube = new Obj3d(cubebp, *renderer);
				cube->local.setPos(-10 + float(i) * -1.1, j * 1.1, 0);
				cube->local.setScale(1, 1, 1);
				cube->setColor(2.5 * i, 0, 0);
				cube->displayTexture = true;
				cube->setTexture(lenatex);
				cube->setPolygonMode(GL_FILL);
				m.renderlist.push_back(cube);
			}
		}
	}
	if (1) {//lambos
		for (size_t k = 0; k < 1; k++) {
			for (size_t j = 0; j < 50; j++) {
				for (size_t i = 0; i < 50; i++) {
					Obj3d* lambo = new Obj3d(lambobp, *renderer);
					lambo->local.setPos(i * 3, j * 1.5, k);
					lambo->local.enlarge(5, 5, 5);
					lambo->setColor(222, 0, 222);
					lambo->setTexture(lambotex);
					lambo->displayTexture = true;
					lambo->setPolygonMode(GL_FILL);
					m.renderlist.push_back(lambo);
				}
			}
		}
	}

	Fps	fps(135);
	Fps* defaultFps = &fps;

	Obj3d* frontobj = static_cast<Obj3d*>(m.renderlist.front());
	Obj3dBP& frontbp = frontobj->getBlueprint();
#ifndef SHADER_INIT
	for (auto o : m.renderlist)
		o->update();
	glUseProgram(renderer->_program);
	glUniform1i(renderer->_dismod, 0);// 1 = display plain_color, 0 = vertex_color
	glUniform3f(renderer->_plain_color, 200, 0, 200);

	glUniform1f(renderer->_tex_coef, 1.0f);
	glBindVertexArray(frontbp.getVao());
	glBindTexture(GL_TEXTURE_2D, lambotex->getId());
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	int	vertices_amount = frontbp.getPolygonAmount() * 3;
#endif
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
	glfwSwapInterval(0);//0 = disable vsynx
	glDisable(GL_CULL_FACE);
	std::cout << "renderlist: " << m.renderlist.size() << "\n";
	std::cout << "Begin while loop\n";
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {
			m.glfw->setTitle(std::to_string(defaultFps->getFps()) + " fps");

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			m.cam->events(*m.glfw, float(defaultFps->getTick()));

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (input[0] >= '3') {//instanced
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else if (1) {
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else {//optimized mass renderer (non instanced) non moving objects, same BP
				Math::Matrix4	VPmatrix(cam.getProjectionMatrix());
				Math::Matrix4	Vmatrix = cam.getViewMatrix();
				VPmatrix.mult(Vmatrix);

				glUseProgram(renderer->_program);
				if (frontobj->displayTexture && frontobj->getTexture() != nullptr) {
					glUniform1f(renderer->_tex_coef, 1.0f);
					glActiveTexture(GL_TEXTURE0);//required for some drivers
					glBindTexture(GL_TEXTURE_2D, frontobj->getTexture()->getId());
				}
				else { glUniform1f(renderer->_tex_coef, 0.0f); }
				glBindVertexArray(frontbp.getVao());

				for (Object* object : m.renderlist) {
					Math::Matrix4	MVPmatrix(VPmatrix);
					MVPmatrix.mult(object->getWorldMatrix());
					MVPmatrix.setOrder(COLUMN_MAJOR);

					glUniformMatrix4fv(renderer->_mat4_mvp, 1, GL_FALSE, MVPmatrix.getData());
					//if (Obj3dBP::defaultDataMode == BP_LINEAR)
					if (frontbp.getDataMode() == BP_LINEAR)
						glDrawArrays(GL_TRIANGLES, 0, vertices_amount);
					else {// should be BP_INDICES
						glDrawElements(GL_TRIANGLES, vertices_amount, GL_UNSIGNED_INT, 0);
					}

				}
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);//this will unbind obj3d pg vao and texture
			glfwSwapBuffers(m.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "End while loop\n";
}

#define M_THREADS_BUILDERS		1
#define M_PERLIN_GENERATION		1
#define M_OCTREE_OPTIMISATION	1
#define M_DISPLAY_BLACK			1
#define M_DRAW_GRID_CHUNK		1
#define M_DRAW_MINIMAP			0

static void		keyCallback_ocTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << "\n";

	float	move = 150;
	OctreeManager* manager = static_cast<OctreeManager*>(glfwGetWindowUserPointer(window));
	if (!manager) {
		std::cout << "static_cast failed\n";
		return;
	}
	if (action == GLFW_PRESS) {
		std::cout << "GLFW_PRESS:" << key << "\n";
		if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER) {
				manager->polygon_mode++;
				manager->polygon_mode = GL_POINT + (manager->polygon_mode % 3);
				for (std::list<Object*>::iterator it = manager->renderlist.begin(); it != manager->renderlist.end(); ++it) {
					((Obj3d*)(*it))->setPolygonMode(manager->polygon_mode);
				}
			}
			else if (key == GLFW_KEY_X) {
				manager->cam->local.translate(move, 0, 0);
			}
			else if (key == GLFW_KEY_Y) {
				manager->cam->local.translate(0, move, 0);
			}
			else if (key == GLFW_KEY_Z) {
				manager->cam->local.translate(0, 0, move);
			}
			else if (key == GLFW_KEY_LEFT_SHIFT) {
				manager->shiftPressed = true;
				manager->cam->speed = manager->playerSpeed * 3;
			}
		}
	}
	else if (action == GLFW_RELEASE) {
		std::cout << "GLFW_RELEASE:" << key << "\n";
		if (manager->glfw) {
			if (key == GLFW_KEY_LEFT_SHIFT) {
				std::cout << "GLFW_KEY_LEFT_SHIFT\n";
				manager->shiftPressed = false;
				manager->cam->speed = manager->playerSpeed;
			}
		}
	}
}

void	grabObjectFromGenerator(ChunkGenerator& generator, OctreeManager& manager, Obj3dBP& cubebp, Obj3dPG& obj3d_prog, Texture* tex_lena) {
	std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";
	std::cout << generator.getGridChecks() << "\n";

#if M_DRAW_MINIMAP == 1
	//assemble minimap (currently inverted on the Y axis)
	float	zmax = generator.gridSize.z * generator.chunkSize.z;//for gl convertion
	for (unsigned int k = 0; k < generator.gridSize.z; k++) {
		for (unsigned int i = 0; i < generator.gridSize.x; i++) {
			if (generator.heightMaps[k][i] && generator.heightMaps[k][i]->panel) {//at this point, the hmap might not be generated yet, nor the panel
				float posx = i * generator.chunkSize.x + 0;
				float posz = k * generator.chunkSize.z + 0;
				float posx2 = posx + generator.chunkSize.x;
				float posz2 = posz + generator.chunkSize.z;
				generator.heightMaps[k][i]->panel->setPos(posx * manager.minimapCoef, (zmax - posz) * manager.minimapCoef);//top left corner
				generator.heightMaps[k][i]->panel->setPos2(posx2 * manager.minimapCoef, (zmax - posz2) * manager.minimapCoef);//bot right corner
			}
		}
	}
	//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiBaseImage);
#endif

	//todo use smart pointers (not for renderlistVoxels)
	for (auto i : manager.renderlist)
		delete i;
	for (auto i : manager.renderlistOctree)
		delete i;
	for (auto i : manager.renderlistGrid)
		delete i;
	//dont delete obj in renderlistVoxels as they're all present and already deleted in renderlist

	manager.renderlist.clear();
	manager.renderlistOctree.clear();
	manager.renderlistGrid.clear();
	for (size_t i = 0; i < 6; i++) {
		manager.renderlistVoxels[i].clear();
		//std::cout << "clear: " << i << "\n";
	}
	manager.renderlistChunk.clear();

	int		hiddenBlocks = 0;
	int		total_polygons = 0;
	int		cubgrid = 0;

	float	scale_coef = 0.99;
	float	scale_coe2 = 0.95;
	//scale_coef = 1;
	std::stringstream polygon_debug;
	polygon_debug << "chunks polygons: ";

	// display box
	Math::Vector3	startDisplay = generator.getGridDisplayStart();
	Math::Vector3	endDisplay = startDisplay + generator.gridDisplaySize;
#if 0 // all the grid
	startDisplay = Math::Vector3();
	endDisplay = Math::Vector3(generator.gridSize);
#endif
	std::cout << " > display " << startDisplay << " -> " << endDisplay << "\n";

	if (0 && M_DRAW_GRID_CHUNK) {
		Math::Vector3 pos(
			int(-generator.gridSize.x / 2),
			int(-generator.gridSize.y / 2),
			int(-generator.gridSize.z / 2));
		pos += generator.gridIndex;
		pos.scale(generator.chunkSize);
		Math::Vector3 scale = generator.gridSize;
		scale.scale(generator.chunkSize);
		Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
		cubeGrid->setColor(255, 0, 0);
		cubeGrid->local.setPos(pos);
		cubeGrid->local.setScale(scale);
		cubeGrid->setPolygonMode(GL_LINE);
		cubeGrid->displayTexture = false;
		manager.renderlistGrid.push_back(cubeGrid);
		cubgrid++;
	}

	for (unsigned int k = startDisplay.z; k < endDisplay.z; k++) {
		for (unsigned int j = startDisplay.y; j < endDisplay.y; j++) {
			for (unsigned int i = startDisplay.x; i < endDisplay.x; i++) {
				Chunk* chunkPtr = generator.grid[k][j][i];
				if (chunkPtr) {//at this point, the chunk might not be generated yet
					if (0) {//coloring origin of each octree/chunk
						Math::Vector3	siz(1, 1, 1);
						Octree* root = chunkPtr->root->getRoot(chunkPtr->root->pos, siz);
						if (root) {
							if (root->size.len() != siz.len())
								root->pixel = Pixel(254, 0, 0);
							else
								root->pixel = Pixel(0, 255, 0);
						}
					}
					if (M_DRAW_GRID_CHUNK) {
						Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
						cubeGrid->setColor(255, 0, 0);
						cubeGrid->local.setPos(chunkPtr->pos);
						cubeGrid->local.setScale(chunkPtr->size * scale_coe2);
						cubeGrid->setPolygonMode(GL_LINE);
						cubeGrid->displayTexture = false;
						manager.renderlistGrid.push_back(cubeGrid);
						cubgrid++;
					}
					if (1) {// one obj3d for the entire chunk
						if (chunkPtr->mesh) {
							manager.renderlistChunk.push_back(chunkPtr->mesh);
							chunkPtr->mesh->setTexture(tex_lena);
							chunkPtr->mesh->displayTexture = false;
							int pol = chunkPtr->mesh->getBlueprint().getPolygonAmount();
							total_polygons += pol;
							polygon_debug << pol << " ";
						}
					}
					else {// oldcode, browsing to build 1 obj3d per cube (not chunk)
						chunkPtr->root->browse(0, [&manager, &cubebp, &obj3d_prog, scale_coef, scale_coe2, chunkPtr, tex_lena, &hiddenBlocks](Octree* node) {
							if (M_DISPLAY_BLACK || (node->pixel.r != 0 && node->pixel.g != 0 && node->pixel.b != 0)) {// pixel 0?
								Math::Vector3	worldPos = chunkPtr->pos + node->pos;
								Math::Vector3	center = worldPos + (node->size / 2);
								if ((node->pixel.r < VOXEL_EMPTY.r \
									|| node->pixel.g < VOXEL_EMPTY.g \
									|| node->pixel.b < VOXEL_EMPTY.b) \
									&& node->neighbors < NEIGHBOR_ALL)// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
								{
									Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
									cube->setColor(node->pixel.r, node->pixel.g, node->pixel.b);
									cube->local.setPos(worldPos);
									cube->local.setScale(node->size.x * scale_coef, node->size.y * scale_coef, node->size.z * scale_coef);
									cube->setPolygonMode(manager.polygon_mode);
									cube->displayTexture = true;
									cube->displayTexture = false;
									cube->setTexture(tex_lena);

									//if (node->neighbors == 1)// faire une texture pour chaque numero
									//	cube->setColor(0, 127, 127);

									//we can see if there is false positive on fully obstructed voxel, some are partially obstructed
									if (node->neighbors == NEIGHBOR_ALL) {//should not be drawn
										cube->setColor(100, 200, 100);
										cube->setPolygonMode(manager.polygon_mode);
										cube->setPolygonMode(GL_FILL);
										hiddenBlocks++;
										manager.renderlist.push_back(cube);
									}
									else {
										// push only the faces next to an EMPTY_VOXEL in an all-in buffer
										manager.renderlist.push_back(cube);
										if ((node->neighbors & NEIGHBOR_FRONT) != NEIGHBOR_FRONT) { manager.renderlistVoxels[CUBE_FRONT_FACE].push_back(cube); }
										if ((node->neighbors & NEIGHBOR_RIGHT) != NEIGHBOR_RIGHT) { manager.renderlistVoxels[CUBE_RIGHT_FACE].push_back(cube); }
										if ((node->neighbors & NEIGHBOR_LEFT) != NEIGHBOR_LEFT) { manager.renderlistVoxels[CUBE_LEFT_FACE].push_back(cube); }
										if ((node->neighbors & NEIGHBOR_BOTTOM) != NEIGHBOR_BOTTOM) { manager.renderlistVoxels[CUBE_BOTTOM_FACE].push_back(cube); }
										if ((node->neighbors & NEIGHBOR_TOP) != NEIGHBOR_TOP) { manager.renderlistVoxels[CUBE_TOP_FACE].push_back(cube); }
										if ((node->neighbors & NEIGHBOR_BACK) != NEIGHBOR_BACK) { manager.renderlistVoxels[CUBE_BACK_FACE].push_back(cube); }
									}
									if (M_DISPLAY_BLACK) {
										Obj3d* cube2 = new Obj3d(cubebp, obj3d_prog);
										cube2->setColor(0, 0, 0);
										cube2->local.setPos(worldPos);
										cube2->local.setScale(node->size.x * scale_coe2, node->size.y * scale_coe2, node->size.z * scale_coe2);
										cube2->setPolygonMode(GL_LINE);
										cube2->displayTexture = false;
										manager.renderlistOctree.push_back(cube2);
									}
								}
							}
						});
					}
				}
			}
		}
	}

	//std::cout << "polygon debug:\n" << polygon_debug.str() << "\n";
	std::cout << "total polygons:\t" << total_polygons << "\n";
	std::cout << "hiddenBlocks:\t" << hiddenBlocks << "\n";
	for (auto i : manager.renderlistVoxels)
		std::cout << "renderlistVoxels[]: " << i.size() << "\n";
	std::cout << "renderlistOctree: " << manager.renderlistOctree.size() << "\n";
	std::cout << "cubes grid : " << cubgrid << "\n";
}

void	scene_octree() {
	//scene_benchmarks(); exit(0);

#ifndef INIT_GLFW
	float	win_height = 900;
	float	win_width = 1600;
	OctreeManager	m;
	std::cout << "cpu core Amount: " << m.cpu_coreAmount << "\n";
	std::this_thread::sleep_for(1s);
	//m.glfw = new Glfw(WINX, WINY);
	m.glfw = new Glfw(win_width, win_height);
	glfwSetWindowPos(m.glfw->_window, 100, 50);

#if 0
	GLint n = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n);
	for (GLint i = 0; i < n; i++) {
		const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
		std::cout << extension << "\n";
	}
#endif
	//exit(0);

	glfwSwapInterval(0);//0 = disable vsynx
	//glDisable(GL_CULL_FACE);

	m.glfw->setTitle("Tests octree");
	m.glfw->activateDefaultCallbacks(&m);
	m.glfw->func[GLFW_KEY_EQUAL] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_MINUS] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_ENTER] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_X] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_Y] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_Z] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_LEFT_SHIFT] = keyCallback_ocTree;

	Texture* tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");
	Texture* tex_lena = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* tex_player = new Texture(SIMPLEGL_FOLDER + "images/player_icon.bmp");

	m.playerMinimap = new UIImage(tex_player);

	//programs
	#if 1 //text
	TextPG::fonts_folder = Misc::getCurrentDirectory() + SIMPLEGL_FOLDER + "fonts/";
	TextPG			rendererText_arial(SIMPLEGL_FOLDER + "shaders/text.vs.glsl", SIMPLEGL_FOLDER + "shaders/text.fs.glsl");
	if (rendererText_arial.init_freetype("arial.ttf", win_width, win_height) == -1) {
		std::cerr << "TextPG::init_freetype failed\n";
		std::exit(-1);
	}
	Text			text1;
	text1.text = "This is sample text";
	text1.color = Math::Vector3(0.5, 0.8f, 0.2f);
	text1.local.setPos(200, 200, 1.0f);
	text1.local.setScale(1, 1, 1);
	Text			text2;
	text2.text = "(C) LearnOpenGL.com";
	text2.color = Math::Vector3(0.3, 0.7f, 0.9f);
	text2.local.setPos(540.0f, 570.0f, 0.5f);
	text2.local.setScale(0.5, 1, 1);
	#endif //text

	Obj3dPG			rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	Obj3dIPG		rendererObj3dInstanced(SIMPLEGL_FOLDER + OBJ3D_INSTANCED_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG		rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);
	Skybox			skybox(*tex_skybox, rendererSkybox);
	m.renderlistSkybox.push_back(&skybox);
	Obj3dPG* renderer = &rendererObj3d;
	//renderer = &rendererObj3dInstanced;
	Chunk::renderer = &rendererObj3d;

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::defaultDataMode = BP_INDICES;

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.speed = m.playerSpeed;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	std::cout << "MouseAngle: " << m.glfw->getMouseAngle() << "\n";
	m.cam = &cam;
#endif //INIT_GLFW
	//QuadNode* root = new QuadNode(baseImage->getData(), w, 0, 0, w, h, THRESHOLD);
	//uint8_t* dataOctree = new uint8_t[w * h * 3];

#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, *renderer);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	//m.renderlist.push_back(&cubeo);

	//Obj3d		player1(cubebp, *rendererObj3d.program);
	//player1.local.setPos(0, 35, 0);
	//player1.local.setScale(1, 2, 1);
	//player1.local.enlarge(5, 5, 5);
	//player1.setColor(255, 0, 0);
	//player1.displayTexture = false;
	//player1.setPolygonMode(GL_FILL);
	//m.player = &player1;

	//std::list<Obj3d*>	playerList;
	//playerList.push_back(&player1);
#endif
#ifndef GENERATOR
	cam.local.setPos(28, 50, 65);//buried close to the surface
	cam.local.setPos(280, 50, 65);//
	//cam.local.setPos(280, -100, 65);//crash
	//cam.local.setPos(-13, 76, 107);//crash
	//chunk generator
	Math::Vector3	playerPos = cam.local.getPos();

	int	g = 5;
	int	d = 3;// g * 2 / 3;
	m.gridSize = Math::Vector3(g, g, g);
	m.gridSizeDisplayed = Math::Vector3(d, d, d);
	ChunkGenerator	generator(playerPos, *m.ps, m.chunk_size, m.gridSize, m.gridSizeDisplayed);

#endif // GENERATOR
	Fps	fps(135);

	//#define MINIMAP // need to build a framebuffer with the entire map, update it each time the player changes chunk
	std::unique_lock<std::mutex> chunks_lock(generator.chunks_mutex, std::defer_lock);
#define USE_THREADSL
#ifdef USE_THREADS
	//chunks builder
	generator.builderAmount = M_THREADS_BUILDERS;
	GLFWwindow* contexts[M_THREADS_BUILDERS];
	std::thread* builders[M_THREADS_BUILDERS];
	for (size_t i = 0; i < generator.builderAmount; i++) {
		contexts[i] = glfwCreateWindow(500, 30, std::to_string(i).c_str(), NULL, m.glfw->_window);
		if (!contexts[i]) {
			std::cout << "Error when creating context " << i << "\n";
			std::exit(5);
		}
		glfwSetWindowPos(contexts[i], 2000, 50 + 30 * i);
		builders[i] = new std::thread(std::bind(&ChunkGenerator::th_builders, &generator, contexts[i]));
	}
	//helper: player pos, jobs, trash(need gl cntext?)
	std::thread helper0(std::bind(&ChunkGenerator::th_updater, &generator, &cam));

	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwFocusWindow(m.glfw->_window);
	std::this_thread::sleep_for(1s);
	std::cout << "notifying threads to build data...\n";
	generator.cv.notify_all();

	unsigned int frames = 0;
	std::cout << "cam: " << cam.local.getPos().toString() << "\n";
	std::cout << "Begin while loop, renderer: " << typeid(renderer).name() << "\n";
	//std::cout.setstate(std::ios_base::failbit);
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			//std::this_thread::sleep_for(1s);
			frames++;
			if (frames % 500 == 0) {
				std::cout << ">>>>>>>>>>>>>>>>>>>> " << frames << " FRAMES <<<<<<<<<<<<<<<<<<<<\n";
				std::cout << "cam: " << cam.local.getPos().toString() << "\n";
			}
			m.glfw->setTitle(std::to_string(fps.getFps()) + " fps");

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			{
				std::lock_guard<std::mutex> guard(generator.mutex_cam);
				m.cam->events(*m.glfw, float(fps.getTick()));
			}
			if (generator.job_mutex.try_lock()) {
				if (generator.chunksChanged && chunks_lock.try_lock()) {
					//std::cout << "[renderer] lock chunks_mutex\n";
					double start = glfwGetTime();
					std::cout << &generator << " : grabbing meshes...\n";
					grabObjectFromGenerator(generator, m, cubebp, *renderer, tex_lena);
					start = glfwGetTime() - start; std::cout << "grabbed in " << start << " seconds\n";
					generator.chunksChanged = false;
					chunks_lock.unlock();
					// the generator can do what he want with the grid, the renderer has what he need for the current frame
				}
				generator.job_mutex.unlock();
				glFinish();
			}

			//GLuint	mode = m.polygon_mode;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//std::cout << "renderlist\n";
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderObjects(m.renderlistVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else if (1) { // converted to mesh
				if (chunks_lock.try_lock()) {//should be another mutex?
					//chunks_lock.lock();
					//std::cout << "Rendering objects... ";
					Chunk::renderer->renderObjects(m.renderlistChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
					//std::cout << "Done\n";
					chunks_lock.unlock();
				}
			}
			//std::cout << "octreeDisplay\n";
			//renderer->renderObjects(m.renderlistOctree, cam, PG_FORCE_DRAW);
#if 1
			glDisable(GL_CULL_FACE);
			renderer->renderObjects(m.renderlistGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
#endif
			rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);

			rendererText_arial.render(text1, Math::Matrix4());
			rendererText_arial.render(text2, Math::Matrix4());

#ifdef MINIMAP
			if (chunks_lock.try_lock() && generator.grid[0][0][0]) {
				std::cout << "[Main] Minimap lock\n";
				//thread player management?
				// coo in 3d world
				playerPos = cam.local.getPos();
				float	zmax = generator.gridSize.z * generator.chunkSize.z;//for gl convertion
				float	px_topleftChunk = generator.grid[0][0][0]->pos.x;//need to setup a mutex or it will crash
				float	pz_topleftChunk = generator.grid[0][0][0]->pos.z;

				// coo on 2d screen
				float	px = (playerPos.x - px_topleftChunk) * m.minimapCoef;
				float	pz = (zmax - (playerPos.z - pz_topleftChunk)) * m.minimapCoef;
				m.playerMinimap->setPos(px, pz);
				m.playerMinimap->setPos2(px + m.playerMinimap->getTexture()->getWidth() * m.minimapCoef,
					pz + m.playerMinimap->getTexture()->getHeight() * m.minimapCoef);
				//build minimap
				//too many blit, build a single image for all the minimap! for all the UI?
				for (unsigned int z = 0; z < generator.gridSize.z; z++) {
					for (unsigned int x = 0; x < generator.gridSize.x; x++) {
						if (generator.heightMaps[z][x])
							blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, generator.heightMaps[z][x]->panel);
					}
				}
				blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, m.playerMinimap);
				std::cout << "[Main] Minimap unlock\n";
				chunks_lock.unlock();
			}
#endif
			glfwSwapBuffers(m.glfw->_window);
			//generator.try_deleteUnusedData();

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}
	std::cout.clear();
	std::cout << "End while loop\n";
	generator.terminateThreads = true;
	generator.cv.notify_all();
	std::cout << "[Main] Notifying cv to wake up waiters, need to join " << int(generator.builderAmount) << " builders...\n";
	for (size_t i = 0; i < generator.builderAmount; i++) {
		//builders[i]->detach();
		builders[i]->join();
		std::cout << "[Main] joined builder " << i << "\n";
	}
	glfwMakeContextCurrent(nullptr);
	helper0.join();
	std::cout << "[Main] joined helper0\n";
	std::cout << "[Main] exiting...\n";
#else  // not USE_THREADS

	//helper: player pos, jobs, trash(need gl cntext?)
	std::thread helper0(std::bind(&ChunkGenerator::th_updater, &generator, &cam));

	std::cout << "Begin while loop\n";
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			//std::cout << ">>>>>>>>>>>>>>>>>>>> NEW FRAME <<<<<<<<<<<<<<<<<<<<\n";
			m.glfw->setTitle(std::to_string(fps.getFps()) + " fps");

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			{
				std::lock_guard<std::mutex> guard(generator.mutex_cam);
				m.cam->events(*m.glfw, float(fps.getTick()));
			}
			//no helper thread
			if (0 && generator.updateGrid(m.cam->local.getPos())) {
				generator.buildMeshesAndMapTiles();
				grabObjectFromGenerator(generator, m, cubebp, *renderer, tex_lena);
			} else if (1) {//with helper thread
				if (!generator.jobsToDo.empty()) {
					generator.build(generator.settings, std::string("[main thread]\t"));
				} else if ((generator.playerChangedChunk || generator.chunksChanged) && chunks_lock.try_lock()){
					//std::cout << "[renderer] lock chunks_mutex\n";
					double start = glfwGetTime();
					std::cout << &generator << " : grabbing meshes...\n";
					grabObjectFromGenerator(generator, m, cubebp, *renderer, tex_lena);
					start = glfwGetTime() - start; std::cout << "grabbed in " << start << " seconds\n";
					if (generator.playerChangedChunk)
						generator.playerChangedChunk = false;
					if (generator.chunksChanged)
						generator.chunksChanged = false;
					chunks_lock.unlock();
				}
			}

			//GLuint	mode = m.polygon_mode;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//std::cout << "renderlist\n";
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderObjects(m.renderlistVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else { // converted to mesh
				Chunk::renderer->renderObjects(m.renderlistChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
			}
			//std::cout << "octreeDisplay\n";
			//renderer->renderObjects(m.renderlistOctree, cam, PG_FORCE_DRAW);
			//std::cout << "gridDisplay\n";
#if true
			glDisable(GL_CULL_FACE);
			renderer->renderObjects(m.renderlistGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
#endif
			rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);

			glfwSwapBuffers(m.glfw->_window);
			//generator.try_deleteUnusedData();

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}
	std::cout << "End while loop\n";
	generator.terminateThreads = true;
	generator.cv.notify_all();
	glfwMakeContextCurrent(nullptr);
	helper0.join();
	std::cout << "[Main] joined helper0\n";
	std::cout << "[Main] exiting...\n";
	std::cout << "[Main] exiting...\n";

#endif // USE_THREADS
}

void	maxUniforms() {
	std::cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS\t" << GL_MAX_VERTEX_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_GEOMETRY_UNIFORM_COMPONENTS\t" << GL_MAX_GEOMETRY_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS\t" << GL_MAX_FRAGMENT_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_COMPUTE_UNIFORM_COMPONENTS\t" << GL_MAX_COMPUTE_UNIFORM_COMPONENTS << "\n";
	std::cout << "GL_MAX_UNIFORM_LOCATIONS\t" << GL_MAX_UNIFORM_LOCATIONS << "\n";
	std::exit(0);
}

void	scene_checkMemory() {
	char input[100];
	Glfw* glfw = new Glfw(WINX, WINY);
	std::string	pathPrefix("SimpleGL/");

	std::cout << "Texture build... ENTER\n";
	std::cin >> input;
	Texture* tex_bigass = new Texture(pathPrefix + "images/skybox4096.bmp");
	std::cout << ">> Texture loaded\n";
	//Textrues
#if 0
	std::cout << "Texture unload load loop... ENTER\n";
	std::cin >> input;

	for (size_t i = 0; i < 100; i++) {
		//unload
		tex_bigass->unloadTexture();
		glfwSwapBuffers(glfw->_window);
		std::cout << ">> Texture unloaded\n";

		//load
		tex_bigass->loadTexture();
		glfwSwapBuffers(glfw->_window);
		std::cout << ">> Texture loaded\n";
	}
#endif
	//Skybox
#if 1
	SkyboxPG	rendererSkybox(pathPrefix + CUBEMAP_VS_FILE, pathPrefix + CUBEMAP_FS_FILE);

	for (size_t i = 0; i < 100; i++) {
		Skybox* sky = new Skybox(*tex_bigass, rendererSkybox);
		delete sky;
	}
#endif
	//framebuffer
#if 1
#endif
//exit
	std::cout << "end... ENTER\n";
	std::cin >> input;
	std::exit(0);

}

#endif //main all

#if 1 main
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//thread safe cout : https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
//multithread monitor example : https://stackoverflow.com/questions/51668477/c-lock-a-mutex-as-if-from-another-thread
int		main(int ac, char **av) {

	//ChunkGenerator::test_calcExceed();std::exit(0);
	//ChunkGenerator::exceedTest();std::exit(0);

	//Math::Vector3	v1(1, 2, 3);
	//Math::Vector3	v2(1, 2, 3);
	//Math::Vector3	v3(0, 2, 3);
	//std::cout << (v1 == v2) << "\n";
	//std::cout << (v2 == v1) << "\n";
	//std::cout << (v1 != v2) << "\n";
	//std::cout << (v2 != v1) << "\n";
	//std::cout << (v2 != v3) << "\n";
	//std::cout << (v2 == v3) << "\n";
	//exit(0);

	//playertest();
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	_CrtMemState sOld;
	_CrtMemState sNew;
	_CrtMemState sDiff;
	_CrtMemCheckpoint(&sOld); //take a snapchot

	//maxUniforms();
	//check_paddings();
	// test_behaviors();
	//test_mult_mat4(); exit(0);
	//	test_obj_loader();

	std::cout << "____START____ :" << Misc::getCurrentDirectory() << "\n";
	//scene1();
	//scene2();
	//scene_4Tree();
	//scene_procedural();
	//scene_vox();
	//testtype(true);	testtype(false); exit(0);
	//scene_benchmarks();
	//scene_checkMemory();
	scene_octree();
	// while(1);

	std::cout << "____END____ : \n";
	_CrtMemCheckpoint(&sNew); //take a snapchot 
	if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
	{
		OutputDebugStringA("-----------_CrtMemDumpStatistics ---------");
		_CrtMemDumpStatistics(&sDiff);
		OutputDebugStringA("-----------_CrtMemDumpAllObjectsSince ---------");
		_CrtMemDumpAllObjectsSince(&sOld);
		OutputDebugStringA("-----------_CrtDumpMemoryLeaks ---------");
		_CrtDumpMemoryLeaks();
	} else {
		std::cout << "no diff for memory check\n";
	}
	return (EXIT_SUCCESS);
}
#endif //main
