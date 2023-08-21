#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1600;
    const int WINDOW_HEIGHT = 900;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao[10];         // Handle for the vertex array object
        GLuint vbo[10];         // Handle for the vertex buffer object
        GLuint nVertices[10];    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture
    GLuint glassOneTextureId;
    GLuint glassTwoTextureId;
    GLuint groundTextureId;
    GLuint skyTextureId;
    GLuint bushTextureId;
    glm::vec2 gUVScale(2.0f, 2.0f);
    glm::vec2 gGROUNDUVScale(10.0f, 10.0f);
    glm::vec2 gSKYUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.2f, 4.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool isPerspective = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
   
    glm::vec3 gGroundPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gGroundScale(3.0f);
    glm::vec3 gBackdropScale(1.0f);
    glm::vec3 gBushScale(0.2f);

    // Tower and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 0.95f);

    // Light position and scale
    glm::vec3 gLightPosition(0.0f, 7.5f, 5.0f);
    glm::vec3 gLightScale(0.7f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Tower Vertex Shader Source Code*/
const GLchar* towerVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Tower Fragment Shader Source Code*/
const GLchar* towerFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing tower color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on tower
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller tower) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);




// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
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

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(towerVertexShaderSource, towerFragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* glassOneFilename = "../../resources/textures/Glass.jpg";
    const char* glassTwoFilename = "../../resources/textures/GlassTwo.jpg";
    const char* groundFilename = "../../resources/textures/natural-stone-aged-paviment.jpg";
    const char* skyFilename = "../../resources/textures/Sky3.jpg";
    const char* bushFilename = "../../resources/textures/Bush.jpg";

    if (!UCreateTexture(glassOneFilename, glassOneTextureId))
    {
        cout << "Failed to load texture " << glassOneFilename << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(glassTwoFilename, glassTwoTextureId))
    {
        cout << "Failed to load texture " << glassOneFilename << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(groundFilename, groundTextureId)) {
        cout << "Failed to load texture " << groundFilename << endl;
        system("PAUSE");
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(skyFilename, skyTextureId)) {
        cout << "Failed to load texture " << skyFilename << endl;
        system("PAUSE");
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(bushFilename, bushTextureId)) {
        cout << "Failed to load texture " << bushFilename << endl;
        system("PAUSE");
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(glassOneTextureId);
    UDestroyTexture(glassTwoTextureId);
    UDestroyTexture(groundTextureId);
    UDestroyTexture(skyTextureId);
    UDestroyTexture(bushTextureId);
    
    // Release shader programs
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

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

    // GLFW: window creation
    // ---------------------
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

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
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
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        isPerspective = false;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        isPerspective = true;

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        gUVScale += 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        gUVScale -= 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
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


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // GROUND: draw ground
    // Activate the ground VAO 
    glBindVertexArray(gMesh.vao[0]);

    //----------------
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gGroundPosition) * glm::scale(gGroundScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a projection
    glm::mat4 projection;

    // Check for isPerspective / Toggle with P/O for Perspective, Ortho 
    if (isPerspective) {
        projection = glm::perspective(glm::radians(gCamera.Zoom),
            (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the tower Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the tower Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    //glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr((gGROUNDUVScale)));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

    // Draw the Sky
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr((gSKYUVScale)));
    model = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, -2.7f, -2.7f)) * 
        glm::scale(glm::vec3(1.0f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the VAO 
    glBindVertexArray(gMesh.vao[6]);
    glBindTexture(GL_TEXTURE_2D, skyTextureId);
    glActiveTexture(GL_TEXTURE0);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[6]);

    
    //TOWER: draw Tower Skinny 
    //gTexWrapMode = GL_REPEAT;
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr((gUVScale)));
    model = glm::translate(glm::vec3(1.6f, 1.5f, -2.2f)) * glm::scale(glm::vec3(0.4f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassOneTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);

    //TOWER: draw Tower Wide 
    model = glm::translate(glm::vec3(0.8f, 1.58f, -2.2f)) * glm::scale(glm::vec3(0.75f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassTwoTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);

    //TOWER: draw Tall Tower Skinny (1)
    model = glm::translate(glm::vec3(-0.6f, 2.3f, -2.2f)) * glm::scale(glm::vec3(0.4f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[3]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassOneTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);

    //TOWER: draw Tall Tower Skinny (2)
    model = glm::translate(glm::vec3(0.1f, 2.1f, -2.2f)) * glm::scale(glm::vec3(0.37f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[3]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassOneTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);


    //TOWER: draw Tower Cap 
    model = glm::translate(glm::vec3(-1.4f, 1.50f, -2.2f)) * glm::scale(glm::vec3(0.40f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[4]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassTwoTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[4]);

    //TOWER: draw Tower Small (1)
    model = glm::translate(glm::vec3(-1.0f, 0.45f, -2.4f)) * glm::scale(glm::vec3(0.30f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[5]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassTwoTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[5]);

    //TOWER: draw Tower Small (2)
    model = glm::translate(glm::vec3(-0.27f, 0.21f, -2.5f)) * glm::scale(glm::vec3(0.20f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Tower VAO 
    glBindVertexArray(gMesh.vao[5]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glassTwoTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[5]);


    //TOWER: draw Bush (1)
    model = glm::translate(glm::vec3(-0.7f, -0.3f, 1.0f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    //TOWER: draw Bush (2)
    model = glm::translate(glm::vec3(-0.9f, -0.3f, 0.3f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    //TOWER: draw Bush (3)
    model = glm::translate(glm::vec3(0.7f, -0.3f, 1.0f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    //TOWER: draw Bush (4)
    model = glm::translate(glm::vec3(0.9f, -0.3f, 0.3f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    //TOWER: draw Bush (5)
    model = glm::translate(glm::vec3(-0.5f, -0.3f, 1.7f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    //TOWER: draw Bush (6)
    model = glm::translate(glm::vec3(0.5f, -0.3f, 1.7f)) * glm::scale(gBushScale);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the Bush VAO 
    glBindVertexArray(gMesh.vao[7]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bushTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    
    glUseProgram(gLampProgramId);
    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles
    //glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{

    // Ground
    GLfloat groundVerts[] = {
    //Positions          //Normals         //Texture Coords.
    // ------------------------------------------------------
    -1.0f, -0.1f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
    1.0f, -0.1f, -1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
    -1.0f, -0.1f, -1.0f, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    -1.0f, -0.1f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
    1.0f, -0.1f, -1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
    1.0f, -0.1f, 1.0f,   0.0f, 1.0f, 0.0f,  1.0f, 0.0f
    };

    // Sky
    GLfloat skyVerts[] = {
        //Positions          //Normals         //Texture Coords.
        // ------------------------------------------------------
        -3.0f, -0.3f, 3.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        3.0f, -0.3f, -3.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -3.0f, -0.3f, -3.0f, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        -3.0f, -0.3f, 3.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        3.0f, -0.3f, -3.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        3.0f, -0.3f, 3.0f,   0.0f, 1.0f, 0.0f,  1.0f, 0.0f
    };

    // Tower Tall Wide  
    GLfloat towerWideVerts[] = {
    //Positions          //Normals         //Texture Coords.
    // ------------------------------------------------------
    //Back Face          //Negative Z Normal 
    -0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
    0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    //Front Face         //Positive Z Normal
    -0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    //Left Face           //Negative X Normal
     -0.5f,  2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     -0.5f,  2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
     -0.5f, -2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
     -0.5f, -2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
     -0.5f, -2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
     -0.5f,  2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    //Right Face         //Positive X Normal
     0.5f,  2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
     0.5f, -2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
     0.5f, -2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
     0.5f, -2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
     0.5f,  2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    //Bottom Face        //Negative Y Normal
    -0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    //Top Face           //Positive Y Normal
   -0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // Tower Skinny 
    GLfloat towerSkinnyVerts[] = {
        //Positions          //Normals         //Texture Coords.
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal 
        -0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        //Front Face         //Positive Z Normal
        -0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
        0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        //Left Face           //Negative X Normal
         -0.5f,  4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
         -0.5f, -4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
         -0.5f, -4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
         -0.5f, -4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
         -0.5f,  4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

         //Right Face         //Positive X Normal
          0.5f,  4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
          0.5f,  4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
          0.5f, -4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
          0.5f, -4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
          0.5f, -4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
          0.5f,  4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

          //Bottom Face        //Negative Y Normal
          -0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
           0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
           0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
           0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
          -0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
          -0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

          //Top Face           //Positive Y Normal
         -0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // Tower Tall Skinny 
    GLfloat towerTallSkinnyVerts[] = {
            //Positions          //Normals         //Texture Coords.
            // ------------------------------------------------------
            //Back Face          //Negative Z Normal 
            -0.5f, -6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
            0.5f, -6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
            0.5f,  6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            0.5f,  6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -6.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

            //Front Face         //Positive Z Normal
            -0.5f, -6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
            0.5f, -6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
            0.5f,  6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            0.5f,  6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
            -0.5f, -6.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

            //Left Face           //Negative X Normal
             -0.5f,  6.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             -0.5f,  6.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
             -0.5f, -6.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
             -0.5f, -6.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
             -0.5f, -6.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
             -0.5f,  6.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

             //Right Face         //Positive X Normal
              0.5f,  6.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
              0.5f,  6.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
              0.5f, -6.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
              0.5f, -6.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
              0.5f, -6.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
              0.5f,  6.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

              //Bottom Face        //Negative Y Normal
              -0.5f, -6.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
               0.5f, -6.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
               0.5f, -6.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
               0.5f, -6.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
              -0.5f, -6.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
              -0.5f, -6.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

              //Top Face           //Positive Y Normal
             -0.5f,  6.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
              0.5f,  6.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
              0.5f,  6.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
              0.5f,  6.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             -0.5f,  6.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
             -0.5f,  6.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // Tower with Cap 
    GLfloat towerCapVerts[] = {
    //Positions          //Normals         //Texture Coords.
    // ------------------------------------------------------
    // Tower Body
    //Back Face          //Negative Z Normal 
    -0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
    0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -4.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    //Front Face         //Positive Z Normal
    -0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -4.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    //Left Face           //Negative X Normal
    -0.5f,  4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
    -0.5f, -4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
    -0.5f, -4.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
    -0.5f, -4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
    -0.5f,  4.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    //Right Face         //Positive X Normal
    0.5f,  4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    0.5f,  4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
    0.5f, -4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
    0.5f, -4.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
    0.5f, -4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
    0.5f,  4.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    //Bottom Face        //Negative Y Normal
    -0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
    0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -4.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -4.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    //Top Face           //Positive Y Normal
    -0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  4.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  4.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,

    // Tower Top
    // Front
    -0.5f, 4.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 5.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.5f, 4.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,

    // Back
    -0.5f, 4.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
     0.0f, 5.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    0.5f, 4.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,

    // Right
   0.5f, 4.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 4.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 5.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

    // Left
   -0.5f, 4.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.5f, 4.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 5.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f

};

    // Small tower
    GLfloat towerSmallVerts[] = {
        //Positions          //Normals         //Texture Coords.
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal 
        -0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -2.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        //Front Face         //Positive Z Normal
        -0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
        0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -2.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        //Left Face           //Negative X Normal
         -0.5f,  2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
         -0.5f, -2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
         -0.5f, -2.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
         -0.5f, -2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
         -0.5f,  2.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

         //Right Face         //Positive X Normal
          0.5f,  2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
          0.5f,  2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
          0.5f, -2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
          0.5f, -2.5f, -0.5f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f,
          0.5f, -2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,
          0.5f,  2.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

          //Bottom Face        //Negative Y Normal
          -0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
           0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
           0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
           0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
          -0.5f, -2.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
          -0.5f, -2.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

          //Top Face           //Positive Y Normal
         -0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f,  2.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  2.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // Bush
    GLfloat bushVerts[] = {
    //Positions          //Normals         //Texture Coords.
    // ------------------------------------------------------
    // Base 
    -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 0.0f, -0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.7f, 0.0f, -0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 0.0f, -0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, -0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,

    // Side #1
    -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.7f, 1.0f, -0.7f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 0.0f, -0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.7f, 1.0f, -0.7f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,

    // Side #2
    -0.7f, 0.0f, -0.7f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    -0.7f, 1.0f, -0.7f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    -0.7f, 0.0f, -0.7f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,

    // Side #3
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    0.7f, 1.0f, -0.7f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    0.7f, 0.0f, -0.7f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    0.7f, 1.0f, -0.7f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,

    // Side #4
    0.7f, 0.0f, -0.7f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 1.0f, -0.7f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, -0.7f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,

    // Side #5
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.7f, 1.0f, 0.7f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.7f, 1.0f, 0.7f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,

    // Side #6
    0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.7f, 1.0f, 0.7f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,

    // Side #7
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    -0.7f, 1.0f, 0.7f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    -0.7f, 0.0f, 0.7f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    -0.7f, 1.0f, 0.7f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,

    // Side #8
    -0.7f, 0.0f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 1.0f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.7f, 0.0f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,

    // Top 
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 1.0f, -0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.7f, 1.0f, -0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 1.0f, -0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.7f, 1.0f, -0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 1.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.7f, 1.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    -0.7f, 1.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.7f, 1.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
  
};
        
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices[0] = sizeof(groundVerts) / (sizeof(groundVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[1] = sizeof(towerWideVerts) / (sizeof(towerWideVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[2] = sizeof(towerSkinnyVerts) / (sizeof(towerSkinnyVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[3] = sizeof(towerTallSkinnyVerts) / (sizeof(towerTallSkinnyVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[4] = sizeof(towerCapVerts) / (sizeof(towerCapVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[5] = sizeof(towerSmallVerts) / (sizeof(towerSmallVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[6] = sizeof(skyVerts) / (sizeof(skyVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[7] = sizeof(bushVerts) / (sizeof(bushVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    

    ////////// Ground Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[0]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[0]);
    glGenBuffers(1, &mesh.vbo[0]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVerts), groundVerts, GL_STATIC_DRAW); // Sendsvertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Sky Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[6]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[6]);
    glGenBuffers(1, &mesh.vbo[6]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[6]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW); // Sendsvertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Tower Wide Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[1]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[1]);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[1]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[1]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(towerWideVerts), towerWideVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Tower Skinny Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[2]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[2]);


    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[2]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[2]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(towerSkinnyVerts), towerSkinnyVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Tower Tall Skinny Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[3]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[3]);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[3]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[3]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(towerTallSkinnyVerts), towerTallSkinnyVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);


    ////////// Tower Cap Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[4]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[4]);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[4]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[4]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(towerCapVerts), towerCapVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Tower Small Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[5]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[5]);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[5]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[5]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(towerSmallVerts), towerSmallVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    ////////// Bush Mesh ////////////
    glGenVertexArrays(1, &mesh.vao[7]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[7]);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo[7]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[7]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(bushVerts), bushVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
    
    
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, mesh.vao);
    glDeleteBuffers(1, mesh.vbo);
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


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
