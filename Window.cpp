#include "window.h"

const char* window_title = "GLFW Starter Project";
Cube * cube;
SkyBox * skybox;
HeightMap* heightmap;

GLint shaderProgram;
GLint skyboxShaderProgram;
GLint normalsShaderProgram;

// On some systems you need to change this to the absolute path
#define VERTEX_SHADER_PATH "../shader.vert"
#define FRAGMENT_SHADER_PATH "../shader.frag"
#define GEOMETRY_SHADER_PATH "../shader.gs"

#define SKY_VERTEX_SHADER_PATH "../skyboxShader.vert"
#define SKY_FRAGMENT_SHADER_PATH "../skyboxShader.frag"
#define SKYBOX_FACE_DIR "../skybox/"
//std::string SKYBOX_DIR = "../skybox/";


// Default camera parameters
glm::vec3 cam_pos(0.0f, 0.0f, 20.0f);		// e  | Position of camera
glm::vec3 cam_look_at(0.0f, 0.0f, 0.0f);	// d  | This is where the camera looks at
glm::vec3 cam_up(0.0f, 1.0f, 0.0f);			// up | What orientation "up" is

int Window::width;
int Window::height;

glm::mat4 Window::P;
glm::mat4 Window::V;

bool Window::lbutton_down;
bool Window::first_time;
glm::vec3 Window::lastPoint;

void Window::initialize_objects()
{
	cube = new Cube();

	std::vector<const GLchar*> faces;
	faces.push_back(SKYBOX_FACE_DIR "right.jpg");
	faces.push_back(SKYBOX_FACE_DIR "left.jpg");
	faces.push_back(SKYBOX_FACE_DIR "top.jpg");
	faces.push_back(SKYBOX_FACE_DIR "bottom.jpg");
	faces.push_back(SKYBOX_FACE_DIR "back.jpg");
	faces.push_back(SKYBOX_FACE_DIR "front.jpg");
	skybox = new SkyBox(faces);

	heightmap = new HeightMap();
	heightmap->loadTextures();
	heightmap->genVertices(30, 30);
	heightmap->init();

	// Load the shader program. Make sure you have the correct filepath up top
	shaderProgram = LoadShaders(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
	skyboxShaderProgram = LoadShaders(SKY_VERTEX_SHADER_PATH, SKY_FRAGMENT_SHADER_PATH);
	normalsShaderProgram = LoadShaders("../normals.vert", "../normals.frag", "../normals.gs");
}

// Treat this as a destructor function. Delete dynamically allocated memory here.
void Window::clean_up()
{
	delete(cube);
	glDeleteProgram(shaderProgram);
}

GLFWwindow* Window::create_window(int width, int height)
{
	// Initialize GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return NULL;
	}

	// 4x antialiasing
	glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__ // Because Apple hates comforming to standards
	// Ensure that minimum OpenGL version is 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Enable forward compatibility and allow a modern OpenGL context
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create the GLFW window
	GLFWwindow* window = glfwCreateWindow(width, height, window_title, NULL, NULL);

	// Check if the window could not be created
	if (!window)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		fprintf(stderr, "Either GLFW is not installed or your graphics card does not support modern OpenGL.\n");
		glfwTerminate();
		return NULL;
	}

	// Make the context of the window
	glfwMakeContextCurrent(window);

	// Set swap interval to 1
	glfwSwapInterval(1);

	// Get the width and height of the framebuffer to properly resize the window
	glfwGetFramebufferSize(window, &width, &height);
	// Call the resize callback to make sure things get drawn immediately
	Window::resize_callback(window, width, height);

	return window;
}

void Window::resize_callback(GLFWwindow* window, int width, int height)
{
	Window::width = width;
	Window::height = height;
	// Set the viewport size. This is the only matrix that OpenGL maintains for us in modern OpenGL!
	glViewport(0, 0, width, height);

	if (height > 0)
	{
		P = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
		V = glm::lookAt(cam_pos, cam_look_at, cam_up);
	}
}

void Window::idle_callback()
{
	// Call the update function the cube
	//cube->update();
}

void Window::display_callback(GLFWwindow* window)
{
	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(skyboxShaderProgram);
	skybox->draw(skyboxShaderProgram);
	
	//If you want to see the normals
	//glUseProgram(normalsShaderProgram);
	//heightmap->drawNormals(normalsShaderProgram);

	glUseProgram(shaderProgram);
	//cube->draw(shaderProgram);
	heightmap->draw(shaderProgram);

	// Gets events, including input such as keyboard and mouse or window resizing
	glfwPollEvents();
	// Swap buffers
	glfwSwapBuffers(window);
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Check for a key press
	if (action == GLFW_PRESS)
	{
		// Check if escape was pressed
		if (key == GLFW_KEY_ESCAPE)
		{
			// Close the window. This causes the program to also terminate.
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
}
void Window::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (GLFW_PRESS == action) {
			lbutton_down = true;
		}
		else if (GLFW_RELEASE == action) {
			lbutton_down = false;
			first_time = true;
		}
	}
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	glm::vec3 curPoint;
	if (lbutton_down) {
		curPoint = getTrackballCoordinates(width, height, glm::vec2(xpos, ypos));
		if (first_time) {
			lastPoint = curPoint;
			first_time = false;
			return;
		}

		float angle;
		// Perform horizontal (y-axis) rotation
		angle = (float)(lastPoint.x - curPoint.x);
		cam_pos = glm::vec3(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(cam_pos, 1.0f));
		cam_up = glm::vec3(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(cam_up, 1.0f));

		//Now rotate vertically based on current orientation
		angle = (float)(curPoint.y - lastPoint.y);
		glm::vec3 axis = glm::cross((cam_pos - cam_look_at), cam_up);
		cam_pos = glm::vec3(glm::rotate(glm::mat4(1.0f), angle, axis) * glm::vec4(cam_pos, 1.0f));
		cam_up = glm::vec3(glm::rotate(glm::mat4(1.0f), angle, axis) * glm::vec4(cam_up, 1.0f));
		
		// Now update the camera
		V = glm::lookAt(cam_pos, cam_look_at, cam_up);
		lastPoint.x = curPoint.x;
		lastPoint.y = curPoint.y;
	}
}

float persp = 45.f;
void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	persp += yoffset / 20.f;
	P = glm::perspective(persp, (float)width / (float)height, 0.1f, 1000.0f);
}