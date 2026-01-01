#include <SFML/Window/ContextSettings.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <vector>
#include <GL/glew.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Mouse.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "model.h"
#include "shader_program.h"
#include <iostream>
#include <optional> 

#define WINDOW_HEIGHT 720
#define WINDOW_WIDTH 1280

using namespace std;
using namespace sf;
using namespace glm;
using namespace Assimp;

// --- ‰ﬁ«ÿ «·‹ Skybox ---
float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
};

GLuint loadCubemap(vector<std::string> faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (unsigned int i = 0; i < faces.size(); i++) {
        Image img;
        if (img.loadFromFile(faces[i])) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
        }
        else { std::cout << "Failed to load: " << faces[i] << std::endl; }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

// œ«·… —”„ «·„—»⁄ ( „  ⁄œÌ·Â« · ﬁ»· ≈Õœ«ÀÌ«  UV „Œ’’…)
void addQuad(vector<vec3>& positions, vector<vec2>& uvs, vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec2 uvMin, vec2 uvMax) {
    positions.push_back(p1); positions.push_back(p2); positions.push_back(p4);
    uvs.push_back(uvMin); uvs.push_back({ uvMax.x, uvMin.y }); uvs.push_back({ uvMin.x, uvMax.y });

    positions.push_back(p2); positions.push_back(p3); positions.push_back(p4);
    uvs.push_back({ uvMax.x, uvMin.y }); uvs.push_back(uvMax); uvs.push_back({ uvMin.x, uvMax.y });
}

// œ«·… ·≈÷«›… „ﬂ⁄» „⁄  ÕœÌœ „‰ÿﬁ… «··Ê‰
// uvMin Ê uvMax ÌÕœœ«‰ √Ì Ã“¡ „‰ «·’Ê—… ”‰” Œœ„
void addBox(vector<vec3>& positions, vector<vec2>& uvs, vec3 center, vec3 size, vec2 uvMin, vec2 uvMax) {
    float w = size.x / 2.0f;
    float h = size.y / 2.0f;
    float d = size.z / 2.0f;

    vec3 p1 = center + vec3(-w, -h, d);
    vec3 p2 = center + vec3(w, -h, d);
    vec3 p3 = center + vec3(w, h, d);
    vec3 p4 = center + vec3(-w, h, d);
    vec3 p5 = center + vec3(-w, -h, -d);
    vec3 p6 = center + vec3(w, -h, -d);
    vec3 p7 = center + vec3(w, h, -d);
    vec3 p8 = center + vec3(-w, h, -d);

    // ‰„—— uvMin Ê uvMax ·ﬂ· ÊÃÂ
    addQuad(positions, uvs, p1, p2, p3, p4, uvMin, uvMax); // Front
    addQuad(positions, uvs, p6, p5, p8, p7, uvMin, uvMax); // Back
    addQuad(positions, uvs, p2, p6, p7, p3, uvMin, uvMax); // Right
    addQuad(positions, uvs, p5, p1, p4, p8, uvMin, uvMax); // Left
    addQuad(positions, uvs, p4, p3, p7, p8, uvMin, uvMax); // Top
    addQuad(positions, uvs, p5, p6, p2, p1, uvMin, uvMax); // Bottom
}

// œ«·… »‰«¡ «·”Ì«—… «·„·Ê‰…
Model createCarModel() {
    vector<vec3> pos;
    vector<vec2> uvs;

    //  ⁄—Ì› „‰«ÿﬁ «·√·Ê«‰ ›Ì «·’Ê—… «· Ì ”‰’‰⁄Â« (Ì”«— √Õ„—° Ì„Ì‰ √”Êœ)
    vec2 redUV_Min = { 0.0f, 0.0f };
    vec2 redUV_Max = { 0.5f, 1.0f }; // «·‰’› «·√Ì”—

    vec2 blackUV_Min = { 0.5f, 0.0f };
    vec2 blackUV_Max = { 1.0f, 1.0f }; // «·‰’› «·√Ì„‰

    // 1. Ã”„ «·”Ì«—… (√Õ„—)
    addBox(pos, uvs, vec3(0.0f, 0.5f, 0.0f), vec3(2.5f, 1.0f, 5.0f), redUV_Min, redUV_Max);

    // 2. «·”ﬁ› (√Õ„—)
    addBox(pos, uvs, vec3(0.0f, 1.3f, -0.2f), vec3(2.0f, 0.8f, 2.5f), redUV_Min, redUV_Max);

    // 3. «·⁄Ã·«  (√”Êœ)
    float wheelY = 0.2f;
    float wheelX = 1.3f;
    float wheelZ = 1.8f;
    vec3 wheelSize = vec3(0.4f, 0.6f, 0.8f);

    addBox(pos, uvs, vec3(wheelX, wheelY, wheelZ), wheelSize, blackUV_Min, blackUV_Max);
    addBox(pos, uvs, vec3(-wheelX, wheelY, wheelZ), wheelSize, blackUV_Min, blackUV_Max);
    addBox(pos, uvs, vec3(wheelX, wheelY, -wheelZ), wheelSize, blackUV_Min, blackUV_Max);
    addBox(pos, uvs, vec3(-wheelX, wheelY, -wheelZ), wheelSize, blackUV_Min, blackUV_Max);

    return Model(pos, uvs);
}

// œ«·… ÃœÌœ… ﬂ·Ì«:  ’‰⁄ ’Ê—…  ·ÊÌ‰ «·”Ì«—… »—„ÃÌ«
GLuint createCarColorTexture() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // ”‰’‰⁄ „’›Ê›… √·Ê«‰ ÌœÊÌ…: 2 »ﬂ”· ⁄—÷ ◊ 1 »ﬂ”· ÿÊ·
    // «·»ﬂ”· «·√Ê·: √Õ„— (255, 0, 0)
    // «·»ﬂ”· «·À«‰Ì: √”Êœ (0, 0, 0) (√Ê —„«œÌ €«„ﬁ ·ÌﬂÊ‰ √Ã„·)
    unsigned char data[] = {
        255, 0, 0, 255,   // Red Pixel (RGBA)
        30, 30, 30, 255   // Dark Grey Pixel (RGBA)
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // ≈⁄œ«œ«  «· ‰⁄Ì„ (Nearest · »ﬁÏ «·√·Ê«‰ Õ«œ… Ê·«  Œ ·ÿ)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint generateTexture(const char* imagePath) {
    Image img; if (!img.loadFromFile(imagePath)) cerr << "Failed to load texture: " << imagePath << endl;
    img.flipVertically();
    GLuint texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    return texture;
}

int main() {
    ContextSettings ctxSettings; ctxSettings.minorVersion = 3; ctxSettings.majorVersion = 3; ctxSettings.depthBits = 24;
    Window window(VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Colored Car", Style::Default, State::Windowed, ctxSettings);
    window.setActive(true);
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);
    if (glewInit() != GLEW_OK) return -1;
    glEnable(GL_DEPTH_TEST);

    GLuint skyboxVAO, skyboxVBO; glGenVertexArrays(1, &skyboxVAO); glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO); glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    vector<std::string> faces{ "posx.jpg", "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg", "negz.jpg" };
    GLuint cubemapTexture = loadCubemap(faces);
    ShaderProgram skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    // --- «·≈⁄œ«œ«  ---
    float groundLevel = 0.0f;
    float salonHalfWidth = 8.0f; float roomWidth = 20.0f; float totalLength = 40.0f; float height = 10.0f;
    float xFarRight = salonHalfWidth + roomWidth; float xFarLeft = -(salonHalfWidth + roomWidth);
    float zFront = totalLength; float zBack = -totalLength;

    // --- »‰«¡ «·„⁄—÷ ---
    // (‰” Œœ„ «·œ«·… «·ﬁœÌ„… ·‹ addQuad Â‰« ·√‰‰« ‰—Ìœ  ﬂ—«— «· ﬂ ‘—)
    auto addQuadRepeated = [&](vector<vec3>& p, vector<vec2>& u, vec3 p1, vec3 p2, vec3 p3, vec3 p4, float repeat) {
        p.push_back(p1); p.push_back(p2); p.push_back(p4);
        u.push_back({ 0.0f, 0.0f }); u.push_back({ repeat, 0.0f }); u.push_back({ 0.0f, repeat });
        p.push_back(p2); p.push_back(p3); p.push_back(p4);
        u.push_back({ repeat, 0.0f }); u.push_back({ repeat, repeat }); u.push_back({ 0.0f, repeat });
        };

    vector<vec3> outerFloorPos; vector<vec2> outerFloorUvs;
    addQuadRepeated(outerFloorPos, outerFloorUvs, { -2000.0f, groundLevel, 2000.0f }, { 2000.0f, groundLevel, 2000.0f }, { 2000.0f, groundLevel, -2000.0f }, { -2000.0f, groundLevel, -2000.0f }, 100.0f);
    Model outerFloorModel(outerFloorPos, outerFloorUvs);

    vector<vec3> innerFloorPos; vector<vec2> innerFloorUvs;
    float innerHeight = groundLevel + 0.01f;
    addQuadRepeated(innerFloorPos, innerFloorUvs, { xFarLeft, innerHeight, zFront }, { xFarRight, innerHeight, zFront }, { xFarRight, innerHeight, zBack }, { xFarLeft, innerHeight, zBack }, 10.0f);
    Model innerFloorModel(innerFloorPos, innerFloorUvs);

    vector<vec3> ceilingPos; vector<vec2> ceilingUvs;
    addQuadRepeated(ceilingPos, ceilingUvs, { xFarLeft, height, zBack }, { xFarRight, height, zBack }, { xFarRight, height, zFront }, { xFarLeft, height, zFront }, 10.0f);
    Model ceilingModel(ceilingPos, ceilingUvs);

    vector<vec3> wallPos; vector<vec2> wallUvs;
    addQuadRepeated(wallPos, wallUvs, { xFarLeft, groundLevel, zBack }, { xFarRight, groundLevel, zBack }, { xFarRight, height, zBack }, { xFarLeft, height, zBack }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { xFarLeft, groundLevel, zFront }, { xFarLeft, height, zFront }, { xFarRight, height, zFront }, { xFarRight, groundLevel, zFront }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { xFarLeft, groundLevel, zFront }, { xFarLeft, groundLevel, zBack }, { xFarLeft, height, zBack }, { xFarLeft, height, zFront }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { xFarRight, groundLevel, zBack }, { xFarRight, groundLevel, zFront }, { xFarRight, height, zFront }, { xFarRight, height, zBack }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { xFarLeft, groundLevel, 0.0f }, { -salonHalfWidth, groundLevel, 0.0f }, { -salonHalfWidth, height, 0.0f }, { xFarLeft, height, 0.0f }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { salonHalfWidth, groundLevel, 0.0f }, { xFarRight, groundLevel, 0.0f }, { xFarRight, height, 0.0f }, { salonHalfWidth, height, 0.0f }, 1.0f);
    float doorWidth = 6.0f; float doorZ = 15.0f;
    addQuadRepeated(wallPos, wallUvs, { -salonHalfWidth, groundLevel, zBack }, { -salonHalfWidth, groundLevel, -(doorZ + doorWidth / 2) }, { -salonHalfWidth, height, -(doorZ + doorWidth / 2) }, { -salonHalfWidth, height, zBack }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { -salonHalfWidth, groundLevel, -(doorZ - doorWidth / 2) }, { -salonHalfWidth, groundLevel, (doorZ - doorWidth / 2) }, { -salonHalfWidth, height, (doorZ - doorWidth / 2) }, { -salonHalfWidth, height, -(doorZ - doorWidth / 2) }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { -salonHalfWidth, groundLevel, (doorZ + doorWidth / 2) }, { -salonHalfWidth, groundLevel, zFront }, { -salonHalfWidth, height, zFront }, { -salonHalfWidth, height, (doorZ + doorWidth / 2) }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { salonHalfWidth, groundLevel, -(doorZ + doorWidth / 2) }, { salonHalfWidth, groundLevel, zBack }, { salonHalfWidth, height, zBack }, { salonHalfWidth, height, -(doorZ + doorWidth / 2) }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { salonHalfWidth, groundLevel, (doorZ - doorWidth / 2) }, { salonHalfWidth, groundLevel, -(doorZ - doorWidth / 2) }, { salonHalfWidth, height, -(doorZ - doorWidth / 2) }, { salonHalfWidth, height, (doorZ - doorWidth / 2) }, 1.0f);
    addQuadRepeated(wallPos, wallUvs, { salonHalfWidth, groundLevel, zFront }, { salonHalfWidth, groundLevel, (doorZ + doorWidth / 2) }, { salonHalfWidth, height, (doorZ + doorWidth / 2) }, { salonHalfWidth, height, zFront }, 1.0f);
    Model wallsModel(wallPos, wallUvs);

    // --- »‰«¡ «·”Ì«—… »—„ÃÌ« ---
    Model carModel = createCarModel();

    // ---  Õ„Ì· «·’Ê— ---
    GLuint texInnerFloor = generateTexture("floor.jpeg");
    GLuint texOuterFloor = generateTexture("grass.jpg");
    GLuint texWall = generateTexture("wall.jpeg");
    GLuint texCeiling = generateTexture("ceiling2.jpeg");

    // ’Ê—… «·”Ì«—… «·„’‰Ê⁄… »—„ÃÌ« (√Õ„— Ê√”Êœ)
    GLuint texCar = createCarColorTexture();

    ShaderProgram shaderProgram("shaders/shader.vert", "shaders/shader.frag");
    GLuint modelLoc = glGetUniformLocation(shaderProgram.getProgram(), "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram.getProgram(), "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram.getProgram(), "perspective");

    mat4 perspectiveMatrix = perspective(45.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 2000.0f);

    float playerHeight = 1.8f;
    vec3 cameraPosition{ 0.0f, groundLevel + playerHeight, 35.0f };
    vec3 cameraFront{ 0.0f, 0.0f, -1.0f };
    vec3 up{ 0.0f, 1.0f, 0.0f };

    float yaw = -90.0f; float pitch = 0.0f; float sensitivity = 0.1f;
    Clock clock; bool running = true;
    Mouse::setPosition(Vector2i(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2), window);

    float carAngle = 0.0f;

    while (running) {
        float deltaTime = clock.restart().asSeconds();
        float cameraSpeed = 10.0f * deltaTime;
        carAngle += deltaTime * 0.5f;

        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) running = false;
            if (Keyboard::isKeyPressed(Keyboard::Key::Escape)) running = false;
            else if (const auto* resized = event->getIf<Event::Resized>()) glViewport(0, 0, resized->size.x, resized->size.y);
        }

        if (window.hasFocus()) {
            Vector2i mousePos = Mouse::getPosition(window);
            Vector2i centerPos = Vector2i(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
            float xOffset = (float)(mousePos.x - centerPos.x) * sensitivity;
            float yOffset = (float)(centerPos.y - mousePos.y) * sensitivity;
            Mouse::setPosition(centerPos, window);
            yaw += xOffset; pitch += yOffset;
            if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
            vec3 front;
            front.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
            front.y = glm::sin(glm::radians(pitch));
            front.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }

        if (Keyboard::isKeyPressed(Keyboard::Key::W)) cameraPosition += cameraSpeed * cameraFront;
        if (Keyboard::isKeyPressed(Keyboard::Key::S)) cameraPosition -= cameraSpeed * cameraFront;
        if (Keyboard::isKeyPressed(Keyboard::Key::A)) cameraPosition -= normalize(cross(cameraFront, up)) * cameraSpeed;
        if (Keyboard::isKeyPressed(Keyboard::Key::D)) cameraPosition += normalize(cross(cameraFront, up)) * cameraSpeed;
        if (Keyboard::isKeyPressed(Keyboard::Key::Space)) cameraPosition += cameraSpeed * up;
        if (Keyboard::isKeyPressed(Keyboard::Key::LShift)) cameraPosition -= cameraSpeed * up;

        if (cameraPosition.y < groundLevel + playerHeight) cameraPosition.y = groundLevel + playerHeight;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram.getProgram());
        mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraFront, up);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &perspectiveMatrix[0][0]);

        // —”„ «·„⁄—÷
        mat4 modelMatrix = mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
        glBindTexture(GL_TEXTURE_2D, texOuterFloor); outerFloorModel.draw();
        glBindTexture(GL_TEXTURE_2D, texInnerFloor); innerFloorModel.draw();
        glBindTexture(GL_TEXTURE_2D, texWall); wallsModel.draw();
        glBindTexture(GL_TEXTURE_2D, texCeiling); ceilingModel.draw();

        // —”„ «·”Ì«—… «·„·Ê‰…
        mat4 carMatrix = mat4(1.0f);
        carMatrix = translate(carMatrix, vec3(18.0f, groundLevel, 20.0f));
        carMatrix = rotate(carMatrix, carAngle, vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &carMatrix[0][0]);
        glBindTexture(GL_TEXTURE_2D, texCar);
        carModel.draw();

        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxShader.getProgram());
        mat4 viewSkybox = mat4(mat3(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader.getProgram(), "view"), 1, GL_FALSE, &viewSkybox[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader.getProgram(), "projection"), 1, GL_FALSE, &perspectiveMatrix[0][0]);
        glBindVertexArray(skyboxVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        window.display();
    }
    return 0;
}
