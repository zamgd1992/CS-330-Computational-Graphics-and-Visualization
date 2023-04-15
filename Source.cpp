#include <iostream>         
#include <cstdlib>         
#include <GL/glew.h>       
#include <GLFW/glfw3.h>     
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <camera.h>

using namespace std;

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Milestone Modular 5"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;   // Number of indices of the mesh
    };

    struct GLDoubleMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    GLMesh lMesh;
    GLMesh planeMesh;
    GLMesh wandBoxMesh;
    GLMesh pagesMesh;
    GLMesh bookCoverMesh;
    GLMesh cylinderMesh;
    GLMesh mugMesh;

    // Textures
    GLuint bookCoverTexture;
    GLuint bookPagesTexture;
    GLuint greenLeatherTexture;
    GLuint woodFloorTexture;
    GLuint wandWoodTexture;
    GLuint mugTexture;

    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLightProgramId;

    // Camera
    Camera gCamera(glm::vec3(0.0f, 1.5f, 7.0f)); // Default camera position
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool gPerspectiveView = true; 

    // Timing
    float gDeltaTime = 0.0f; 
    float gLastFrame = 0.0f;

    // Object color
    glm::vec3 gObjectColor(1.0f, 1.0f, 1.0f); 

    // Light properties
    glm::vec3 headLightPosition(0.0f, 0.0f, 0.0f); 
    glm::vec3 headLightColor(1.0f, 1.0f, 1.0f); 

    glm::vec3 sideLightColor(1.0f, 1.0f, 1.0f); 
    glm::vec3 sideLightPosition(0.0f, 3.0f, 7.0f);
    glm::vec3 gLightScale(0.2f);
}

bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void processView(GLFWwindow* window); 
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void createPlaneMesh(GLMesh& mesh);
void createWandboxMesh(GLMesh& mesh);
void createPagesMesh(GLMesh& mesh);
void createBookCoverMesh(GLMesh& mesh);
void createWandMesh(GLMesh& mesh);
void createMugMesh(GLMesh& mesh);
void UCreateLightMesh(GLMesh& mesh);
void URender(); 
void UDrawLightSources();
void drawPlane(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); 
void drawWandbox(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); 
void drawPages(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); 
void drawCover(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); 
void drawWand(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle);
void drawMug(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

// Vertex shader source code
const GLchar* vertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal;
    out vec3 vertexFragmentPos;
    out vec2 vertexTextureCoordinate;

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0f);

        vertexFragmentPos = vec3(model * vec4(position, 1.0f));

        vertexNormal = mat3(transpose(inverse(model))) * normal;
        vertexTextureCoordinate = textureCoordinate;
    }
);


// Fragment shader source code
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal;
    in vec3 vertexFragmentPos;
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture;
    uniform vec2 uvScale;

    void main() {
        float ambientStrength = 0.5f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        // Diffuse calculation
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        // Specular calculation
        float specularIntensity = 0.2f; // Set specular light strength
        float highlightSize = 12.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);



// Light shader source code
const GLchar* lightVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    // Uniform / Global variables for transform matrix
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Trasnform vertices into clip coordinates
    }
);



// Light fragment shader source code
const GLchar* lightFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing light color

    void main() {
        fragmentColor = vec4(1.0f); // Color set to white
    }
);



// flip y axis
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the meshes
    createPlaneMesh(planeMesh);
    createWandboxMesh(wandBoxMesh);
    createPagesMesh(pagesMesh);
    createBookCoverMesh(bookCoverMesh);
    createWandMesh(cylinderMesh);
    createMugMesh(mugMesh);
    UCreateLightMesh(lMesh);

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;


    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;


    // Load textures
    const char* filenameBookCover = ".\\textures\\DarkBlue.jpg";
    const char* filenamePages = ".\\textures\\pagesTexture.jpg";
    const char* filenameGreenLeather = ".\\textures\\greenLeather.png";
    const char* filenameWoodFloor = ".\\textures\\lightWoodFlooring.jpg";
    const char* filenameDarkWood = ".\\textures\\DarkWood.jpg";
    const char* filenameCeramic = ".\\textures\\ceramicTexture.jpg";

    // Check if textures loaded
    if (!UCreateTexture(filenameBookCover, bookCoverTexture)) {
        cout << "Failed to load texture: " << filenameBookCover << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenamePages, bookPagesTexture)) {
        cout << "Failed to load texture: " << filenamePages << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameGreenLeather, greenLeatherTexture)) {
        cout << "Failed to load texture: " << filenameGreenLeather << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameWoodFloor, woodFloorTexture)) {
        cout << "Failed to load texture: " << filenameWoodFloor << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameDarkWood, wandWoodTexture)) {
        cout << "Failed to load texture: " << filenameDarkWood << endl;
        return EXIT_FAILURE;

    }

    if (!UCreateTexture(filenameCeramic, mugTexture)) {
        cout << "Failed to load texture: " << filenameCeramic << endl;
        return EXIT_FAILURE;
    }

    // Tell OpenGL for each sampler to which texture unit it belongs to. 
    glUseProgram(gProgramId);

    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Render loop
    while (!glfwWindowShouldClose(gWindow))
    {
        // Frame timing
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // Inputs
        UProcessInput(gWindow);
        processView(gWindow);

        // Render current frame
        URender();
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(planeMesh);
    UDestroyMesh(wandBoxMesh);
    UDestroyMesh(pagesMesh);
    UDestroyMesh(bookCoverMesh);
    UDestroyMesh(cylinderMesh);
    UDestroyMesh(mugMesh);
    UDestroyMesh(lMesh);

    // Release texture
    UDestroyTexture(bookCoverTexture);
    UDestroyTexture(bookPagesTexture);
    UDestroyTexture(greenLeatherTexture);
    UDestroyTexture(woodFloorTexture);
    UDestroyTexture(wandWoodTexture);
    UDestroyTexture(mugTexture);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLightProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
}

void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; 

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}


void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Changes the view
void processView(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        //std::cout << "P was pressed! Switching to ORTHO mode." << std::endl;
        gPerspectiveView = false;
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        //std::cout << "O was pressed! Switching to PERSPECTIVE mode." << std::endl;
        gPerspectiveView = true;
    }
}


// Update camera
void updateCamera(glm::mat4 model) {
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection; 

    if (gPerspectiveView) {
        // Creates perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        // Creates ortho projection
        projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
    }

    // Shader selection
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, headLightColor.r, headLightColor.g, headLightColor.b);
    glUniform3f(lightPositionLoc, headLightPosition.x, headLightPosition.y, headLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, sideLightColor.r, sideLightColor.g, sideLightColor.b);
    glUniform3f(lightPositionLoc, sideLightPosition.x, sideLightPosition.y, sideLightPosition.z);
}


// Renders

void UDrawLightSources() {
    glUseProgram(gProgramId);

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(0.5, 0.5, 0.5));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.5f, 0.5f, 0.5f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(0.1, 0.1, 0.1));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection; // Initialize projection


    // Determine whether the projection is perspective or ortho.
    if (gPerspectiveView) {
        // Creates perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        // Creates ortho projection
        projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
    }
    // Update camera
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(lMesh.vao);

    // Draw the light source
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;

    // Select shader program
    glUseProgram(gLightProgramId);

    model = glm::translate(sideLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Light Shader program
    modelLoc = glGetUniformLocation(gLightProgramId, "model");
    viewLoc = glGetUniformLocation(gLightProgramId, "view");
    projLoc = glGetUniformLocation(gLightProgramId, "projection");

    // Pass matrix data to the Light Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, lMesh.nVertices);
};

void drawPlane(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); 

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(1.0f, 1.0f, 1.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Update camera transformation
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glBindVertexArray(planeMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodFloorTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, planeMesh.nVertices);
}

void drawPages(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection; // Initialize projection

    // Update camera
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 0.1f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(pagesMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bookPagesTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, pagesMesh.nVertices);
}


void drawCover(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(bookCoverMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bookCoverTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, bookCoverMesh.nVertices);
}


void drawWandbox(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Update the camera's position
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(wandBoxMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, greenLeatherTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, wandBoxMesh.nVertices);
}

void drawMug(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId);

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mugMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mugTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, mugMesh.nVertices);
}

void drawWand(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId);

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.3f, -1.3f, 1.2f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(cylinderMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wandWoodTexture);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, cylinderMesh.nVertices);
}

// Function to draw all the shapes
void URender() {
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Light
    UDrawLightSources();

    // Floor
    drawPlane(5.0, 1.0, 5.0, 0.0, -0.01, 0.0, 0.0);

    // Wandbox
    drawWandbox(.75, 1.0, 3.0, 0.0, 0.0, 0.0, 5.0);

    // Book
    drawCover(1.0, 1.015, 1.02, -2.5, 0.0, -2.0, 14.0); 
    drawPages(0.5, 3.0, 1.0, -2.5, 0.02, -2.0, 14.0); 

    // Wand
    drawWand(0.05, 4.0, 0.05, 2.0, 0.07, 3.0, 15.0);
    drawWand(0.07, 1.0, 0.07, 2.0, 0.07, 3.0, 15.0);
    drawWand(0.09, 0.02, 0.09, 2.0, 0.07, 3.0, 15.0);

    //Mug
    drawMug(0.35, 1.0, 0.35, 0.25, 0.0, -2.0, 0.0);

    // Deactviate VAO
    glBindVertexArray(0);
    glUseProgram(0);

    // Refresh the screen
    glfwSwapBuffers(gWindow);
}

// Meshes

// Template for creating a cube light
void UCreateLightMesh(GLMesh& mesh)
{
    // Vertex Data
    GLfloat verts[] = {
       -0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 0.0f,
        0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
        0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 1.0f,
       -0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 0.0f,
       -0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 1.0f,
        0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 1.0f,

       -0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
        0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
        0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
       -0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
       -0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
        0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,

       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
        0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
       -0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 1.0f,
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,

       -0.5f,  1.0f, -0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
       -0.5f,  0.0f, -0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
       -0.5f,  0.0f,  0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
       -0.5f,  1.0f, -0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
       -0.5f,  1.0f,  0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
       -0.5f,  0.0f,  0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,

       0.5f,  0.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
       0.5f,  1.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
       0.5f,  1.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
       0.5f,  0.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
       0.5f,  0.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
       0.5f,  1.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,

      -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
       0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
       0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
      -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
      -0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
       0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void createPlaneMesh(GLMesh& mesh) {
    GLfloat verts[] = {
       -1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        1.0f,  0.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
       -1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
       -1.0f,  0.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        1.0f,  0.0f, -1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


void createWandboxMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // bottom
       -0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,    0.0f, 1.0f,    
       -0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,    0.0f, 0.0f,    
        0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,    1.0f, 1.0f,    
       -0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,    0.0f, 0.0f,    
        0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,    1.0f, 1.0f,    
        0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,    1.0f, 0.0f,    

        // front
       -0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,    
       -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,    
        0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,    
        0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,    
        0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,    
       -0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,    

       // top
      -0.5f,  0.5f, -0.5f,   0.0f,  1.0f, 0.0f,  0.0f, 1.0f,    
      -0.5f,  0.5f,  0.5f,   0.0f,  1.0f, 0.0f,  0.0f, 0.0f,    
       0.5f,  0.5f, -0.5f,   0.0f,  1.0f, 0.0f,  1.0f, 1.0f,    
       0.5f,  0.5f, -0.5f,   0.0f,  1.0f, 0.0f,  1.0f, 1.0f,   
      -0.5f,  0.5f,  0.5f,   0.0f,  1.0f, 0.0f,  0.0f, 0.0f,   
       0.5f,  0.5f,  0.5f,   0.0f,  1.0f, 0.0f,  1.0f, 0.0f,    

       // left side
       -0.5f,  0.0f,  0.5f,   -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,    
       -0.5f,  0.0f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   
       -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
       -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
       -0.5f,  0.0f,  0.5f,   -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,    
       -0.5f,  0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,    


       // right side
       0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,    
       0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,    
       0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,  
       0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   
       0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,    
       0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,    


       //back
      -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,   
       0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    
      -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,    
      -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,  
       0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,   
       0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,   
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); 

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


void createPagesMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // bottom of pages
       -1.0f,  0.0f, -1.0f,   0.0f, -1.0f, 0.0f,    0.0f, 1.0f,    
       -1.0f,  0.0f,  1.0f,   0.0f, -1.0f, 0.0f,    0.0f, 0.0f,   
        1.0f,  0.0f, -1.0f,   0.0f, -1.0f, 0.0f,    1.0f, 1.0f,   
       -1.0f,  0.0f,  1.0f,   0.0f, -1.0f, 0.0f,    0.0f, 0.0f,
        1.0f,  0.0f, -1.0f,   0.0f, -1.0f, 0.0f,    1.0f, 1.0f,
        1.0f,  0.0f,  1.0f,   0.0f, -1.0f, 0.0f,    1.0f, 0.0f,

        // front of pages
       -1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
       -1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
       -1.0f,  0.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

        // top of pages
       -1.0f,  1.0f, -1.0f,   0.0f,  1.0f, 0.0f,  0.0f, 0.25f,
       -1.0f,  1.0f,  1.0f,   0.0f,  1.0f, 0.0f,  0.0f, 0.75f,
        1.0f,  1.0f, -1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.25f,
        1.0f,  1.0f, -1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.25f,
       -1.0f,  1.0f,  1.0f,   0.0f,  1.0f, 0.0f,  0.0f, 0.75f,
        1.0f,  1.0f,  1.0f,   0.0f,  1.0f, 0.0f,  1.0f, 0.75f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void createBookCoverMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // left side of book
       -0.5f,  0.0f,  1.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,    
       -0.5f,  0.0f, -1.0f,   -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   
       -0.5f,  3.0f, -1.0f,   -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
       -0.5f,  3.0f, -1.0f,   -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
       -0.5f,  0.0f,  1.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,    
       -0.5f,  3.0f,  1.0f,   -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,    

        // right side of book
        0.5f,  0.0f,  1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,    
        0.5f,  0.0f, -1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,    
        0.5f,  3.0f, -1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
        0.5f,  3.0f, -1.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,    
        0.5f,  0.0f,  1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   
        0.5f,  3.0f,  1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  

        // spine of book
       -0.5f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    
        0.5f,  3.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 
       -0.5f,  3.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,   
       -0.5f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    
        0.5f,  3.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    
        0.5f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,    
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void createMugMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // Base 

        -1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,    0.0f, -1.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.0f,    0.0f, -1.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,    0.0f,  1.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

         0.0f,    0.0f,  1.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,

        -0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.5f, 0.0f,


         // Side 1
         0.0f,    0.0f,  1.0f,     1.0f, 0.0f,  1.0f,    0.0f, 0.0f,
         0.0f,    1.0f,  1.2f,     1.0f, 0.0f,  1.0f,    0.0f, 1.0f,
         0.371f,  1.0f,  1.141f,   1.0f, 0.0f,  1.0f,    1.0f, 1.0f,

         0.0f,    0.0f,  1.0f,     1.0f, 0.0f,  1.0f,    0.0f, 0.0f,
         0.371f,  1.0f,  1.141f,   1.0f, 0.0f,  1.0f,    1.0f, 1.0f,
         0.309f,  0.0f,  0.951f,   1.0f, 0.0f,  1.0f,    1.0f, 0.0f,

         // Side 2
         0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.371f,  1.0f,  1.141f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 3
         0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 4
         0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 5
         0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         1.2f,    1.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.2f,    1.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 6
         1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.2f,    1.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 7
         0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 8
         0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 9
         0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 10
         0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,    1.0f, -1.2f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.0f,    1.0f, -1.2f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f, -1.0f,      1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 11
         0.0f,    0.0f, -1.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.0f,    1.0f, -1.2f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.0f,    0.0f, -1.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 12
        -0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.371f,  1.0f, -1.141f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.309f,  0.0f, -0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 13
        -0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.706f,  1.0f, -0.971f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.588f,  0.0f, -0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 14
        -0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.971f,  1.0f, -0.706f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         
        -0.809f,  0.0f, -0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 15
        -0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.141f,  1.0f, -0.371f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -1.2f,    1.0f, 0.0f,       1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.951f,  0.0f, -0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.2f,    1.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 16
        -1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.2f,    1.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -1.0f,    0.0f,  0.0f,      1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,
         
         // Side 17
        -0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.141f,  1.0f,  0.371f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.951f,  0.0f,  0.309f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 18
        -0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.971f,  1.0f,  0.706f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.809f,  0.0f,  0.588f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 19
        -0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.706f,  1.0f,  0.971f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.371f,  1.0f,  1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.588f,  0.0f,  0.809f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.371f,  1.0f,  1.141f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        -0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f,

         // Side 20
        -0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.371f,  1.0f,  1.141f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,    1.0f,  1.2f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.309f,  0.0f,  0.951f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.0f,    1.0f,  1.2f,      1.0f, 0.0f, 1.0f,    1.0f, 1.0f,
         0.0f,    0.0f,  1.0f,      1.0f, 0.0f, 1.0f,    1.0f, 0.0f,
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;


    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void createWandMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // Base 

        -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.0f,  0.0f, -1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.0f,  0.0f, -1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.0f,  0.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

         0.0f,  0.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

        -0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,


         // Side 1
        -1.0f,  0.0f,  0.0f,   -1.0f, 0.0f,  1.0f,    0.0f, 0.0f,
        -1.0f,  1.0f,  0.0f,   -1.0f, 0.0f,  1.0f,    0.0f, 1.0f,
        -0.7f,  1.0f, -0.7f,   -1.0f, 0.0f,  1.0f,    1.0f, 1.0f,

        -1.0f,  0.0f,  0.0f,   -1.0f, 0.0f,  1.0f,    0.0f, 0.0f,
        -0.7f,  0.0f, -0.7f,   -1.0f, 0.0f,  1.0f,    0.0f, 1.0f,
        -0.7f,  1.0f, -0.7f,   -1.0f, 0.0f,  1.0f,    1.0f, 1.0f,

        // Side 2
       -0.7f,  0.0f, -0.7f,    1.0f, 0.0f, -1.0f,    0.0f, 0.0f,
       -0.7f,  1.0f, -0.7f,    1.0f, 0.0f, -1.0f,    0.0f, 1.0f,
        0.0f,  1.0f, -1.0f,    1.0f, 0.0f, -1.0f,    1.0f, 1.0f,

       -0.7f,  0.0f, -0.7f,    1.0f, 0.0f, -1.0f,    0.0f, 0.0f,
        0.0f,  0.0f, -1.0f,    1.0f, 0.0f, -1.0f,    0.0f, 1.0f,
        0.0f,  1.0f, -1.0f,    1.0f, 0.0f, -1.0f,    1.0f, 1.0f,

        // Side 3
       0.0f,  0.0f, -1.0f,    1.0f, 0.0f, -1.0f,    0.0f, 0.0f,
       0.0f,  1.0f, -1.0f,    1.0f, 0.0f, -1.0f,    0.0f, 1.0f,
       0.7f,  1.0f, -0.7f,    1.0f, 0.0f, -1.0f,    1.0f, 1.0f,

       0.0f,  0.0f, -1.0f,    1.0f, 0.0f, -1.0f,    0.0f, 0.0f,
       0.7f,  0.0f, -0.7f,    1.0f, 0.0f, -1.0f,    0.0f, 1.0f,
       0.7f,  1.0f, -0.7f,    1.0f, 0.0f, -1.0f,    1.0f, 1.0f,

       // Side 4
       0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
       0.7f,  1.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
       1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

       0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
       1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
       1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

       // Side 5
      1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
      1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
      0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

      1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
      0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
      0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

      // Side 6
     0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
     0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
     0.0f,  1.0f,  1.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

     0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
     0.0f,  0.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
     0.0f,  1.0f,  1.0f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

     // Side 7
     0.0f,  0.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
     0.0f,  1.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    -0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

     0.0f,  0.0f,  1.0f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
    -0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    -0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

    // Side 8
    -0.7f,  0.0f,  0.7f,    -1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
    -0.7f,  1.0f,  0.7f,    -1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    -1.0f,  1.0f,  0.0f,    -1.0f, 0.0f, 1.0f,    1.0f, 1.0f,

    -0.7f,  0.0f,  0.7f,    -1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
    -1.0f,  0.0f,  0.0f,    -1.0f, 0.0f, 1.0f,    0.0f, 1.0f,
    -1.0f,  1.0f,  0.0f,    -1.0f, 0.0f, 1.0f,    1.0f, 1.0f,


    // Top 
   -1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,   
   -0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,  
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,   

   -0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     
    0.0f,  1.0f, -1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,    
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,   

    0.0f,  1.0f, -1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    
    0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,    
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,    

    0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    
    1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,   
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     

    1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    
    0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,    

    0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    
    0.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,   
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,   

    0.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    
   -0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     

   -0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     
   -1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     
    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;


    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

// Destroy mesh
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


// Destroy Texture
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

// End shader program
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

