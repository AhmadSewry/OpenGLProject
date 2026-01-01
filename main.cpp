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

void addQuad(vector<vec3>& positions, vector<vec2>& uvs,
    vec3 p1, vec3 p2, vec3 p3, vec3 p4) {
    positions.push_back(p1); positions.push_back(p2); positions.push_back(p4);
    uvs.push_back({ 0.0f, 0.0f }); uvs.push_back({ 10.0f, 0.0f }); uvs.push_back({ 0.0f, 10.0f });
    positions.push_back(p2); positions.push_back(p3); positions.push_back(p4);
    uvs.push_back({ 10.0f, 0.0f }); uvs.push_back({ 10.0f, 10.0f }); uvs.push_back({ 0.0f, 10.0f });
}

GLuint generateTexture(const char* imagePath) {
    Image img;
    if (!img.loadFromFile(imagePath)) cerr << "Failed to load texture: " << imagePath << endl;
    img.flipVertically();
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    return texture;
}

Model loadModelFromFile(const char* meshPath) {
    Importer importer;
    const aiScene* scene = importer.ReadFile(meshPath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || !scene->mMeshes[0]) return Model({}, {});
    aiMesh* mesh = scene->mMeshes[0];
    vector<vec3> positions; vector<vec2> uvs;
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        for (int j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
            auto position = mesh->mVertices[mesh->mFaces[i].mIndices[j]];
            positions.push_back({ position.x, position.y, position.z });
            if (mesh->mTextureCoords[0]) uvs.push_back({ mesh->mTextureCoords[0][mesh->mFaces[i].mIndices[j]].x, mesh->mTextureCoords[0][mesh->mFaces[i].mIndices[j]].y });
            else uvs.push_back({ 0.0f, 0.0f });
        }
    }
    return Model(positions, uvs);
}

int main() {
    ContextSettings ctxSettings;
    ctxSettings.minorVersion = 3; ctxSettings.majorVersion = 3; ctxSettings.depthBits = 24;
    Window window(VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "FPS Camera Showroom", Style::Default, State::Windowed, ctxSettings);
    window.setActive(true);

    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (glewInit() != GLEW_OK) return -1;
    glEnable(GL_DEPTH_TEST);

    float salonHalfWidth = 8.0f;  float roomWidth = 20.0f; float totalLength = 40.0f; float height = 8.0f;
    float xFarRight = salonHalfWidth + roomWidth; float xFarLeft = -(salonHalfWidth + roomWidth); float zFront = totalLength; float zBack = -totalLength;

    vector<vec3> floorPos; vector<vec2> floorUvs;
    addQuad(floorPos, floorUvs, { xFarLeft, -2.0f, zFront }, { xFarRight, -2.0f, zFront }, { xFarRight, -2.0f, zBack }, { xFarLeft, -2.0f, zBack });
    Model floorModel(floorPos, floorUvs);

    vector<vec3> ceilingPos; vector<vec2> ceilingUvs;
    addQuad(ceilingPos, ceilingUvs, { xFarLeft, height, zBack }, { xFarRight, height, zBack }, { xFarRight, height, zFront }, { xFarLeft, height, zFront });
    Model ceilingModel(ceilingPos, ceilingUvs);

    vector<vec3> wallPos; vector<vec2> wallUvs;
    addQuad(wallPos, wallUvs, { xFarLeft, -2.0f, zBack }, { xFarRight, -2.0f, zBack }, { xFarRight, height, zBack }, { xFarLeft, height, zBack });
    addQuad(wallPos, wallUvs, { xFarLeft, -2.0f, zFront }, { xFarLeft, height, zFront }, { xFarRight, height, zFront }, { xFarRight, -2.0f, zFront });
    addQuad(wallPos, wallUvs, { xFarLeft, -2.0f, zFront }, { xFarLeft, -2.0f, zBack }, { xFarLeft, height, zBack }, { xFarLeft, height, zFront });
    addQuad(wallPos, wallUvs, { xFarRight, -2.0f, zBack }, { xFarRight, -2.0f, zFront }, { xFarRight, height, zFront }, { xFarRight, height, zBack });
    addQuad(wallPos, wallUvs, { xFarLeft, -2.0f, 0.0f }, { -salonHalfWidth, -2.0f, 0.0f }, { -salonHalfWidth, height, 0.0f }, { xFarLeft, height, 0.0f });
    addQuad(wallPos, wallUvs, { salonHalfWidth, -2.0f, 0.0f }, { xFarRight, -2.0f, 0.0f }, { xFarRight, height, 0.0f }, { salonHalfWidth, height, 0.0f });

    float doorWidth = 6.0f; float doorZ = 15.0f;
    addQuad(wallPos, wallUvs, { -salonHalfWidth, -2.0f, zBack }, { -salonHalfWidth, -2.0f, -(doorZ + doorWidth / 2) }, { -salonHalfWidth, height, -(doorZ + doorWidth / 2) }, { -salonHalfWidth, height, zBack });
    addQuad(wallPos, wallUvs, { -salonHalfWidth, -2.0f, -(doorZ - doorWidth / 2) }, { -salonHalfWidth, -2.0f, (doorZ - doorWidth / 2) }, { -salonHalfWidth, height, (doorZ - doorWidth / 2) }, { -salonHalfWidth, height, -(doorZ - doorWidth / 2) });
    addQuad(wallPos, wallUvs, { -salonHalfWidth, -2.0f, (doorZ + doorWidth / 2) }, { -salonHalfWidth, -2.0f, zFront }, { -salonHalfWidth, height, zFront }, { -salonHalfWidth, height, (doorZ + doorWidth / 2) });
    addQuad(wallPos, wallUvs, { salonHalfWidth, -2.0f, -(doorZ + doorWidth / 2) }, { salonHalfWidth, -2.0f, zBack }, { salonHalfWidth, height, zBack }, { salonHalfWidth, height, -(doorZ + doorWidth / 2) });
    addQuad(wallPos, wallUvs, { salonHalfWidth, -2.0f, (doorZ - doorWidth / 2) }, { salonHalfWidth, -2.0f, -(doorZ - doorWidth / 2) }, { salonHalfWidth, height, -(doorZ - doorWidth / 2) }, { salonHalfWidth, height, (doorZ - doorWidth / 2) });
    addQuad(wallPos, wallUvs, { salonHalfWidth, -2.0f, zFront }, { salonHalfWidth, -2.0f, (doorZ + doorWidth / 2) }, { salonHalfWidth, height, (doorZ + doorWidth / 2) }, { salonHalfWidth, height, zFront });
    Model wallsModel(wallPos, wallUvs);

    GLuint texFloor = generateTexture("floor.jpeg");
    GLuint texWall = generateTexture("wall.jpeg");
    GLuint texCeiling = generateTexture("ceiling2.jpeg");

    ShaderProgram shaderProgram("shaders/shader.vert", "shaders/shader.frag");
    GLuint modelMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "model");
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "view");
    GLuint perspectiveMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "perspective");
    glUseProgram(shaderProgram.getProgram());

    mat4 perspectiveMatrix = perspective(45.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 200.0f);
    glUniformMatrix4fv(perspectiveMatrixLocation, 1, GL_FALSE, &perspectiveMatrix[0][0]);

    vec3 cameraPosition{ 0.0f, 0.0f, 35.0f };
    vec3 cameraFront{ 0.0f, 0.0f, -1.0f };
    vec3 up{ 0.0f, 1.0f, 0.0f };

    float yaw = -90.0f;
    float pitch = 0.0f;
    float sensitivity = 0.1f;

    Clock clock;
    bool running = true;

    Mouse::setPosition(Vector2i(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2), window);

    while (running) {
        float deltaTime = clock.restart().asSeconds();
        float cameraSpeed = 10.0f * deltaTime;

        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) running = false;
            if (Keyboard::isKeyPressed(Keyboard::Key::Escape)) running = false;
            else if (const auto* resized = event->getIf<Event::Resized>())
                glViewport(0, 0, resized->size.x, resized->size.y);
        }

        if (window.hasFocus()) {
            Vector2i mousePos = Mouse::getPosition(window);
            Vector2i centerPos = Vector2i(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

            float xOffset = (float)(mousePos.x - centerPos.x);
            float yOffset = (float)(centerPos.y - mousePos.y);

            Mouse::setPosition(centerPos, window);

            xOffset *= sensitivity;
            yOffset *= sensitivity;

            yaw += xOffset;
            pitch += yOffset;

            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            vec3 front;
            // --- åäÇ ÇáÊÕÍíÍ ---
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

        mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraFront, up);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 modelMatrix = mat4(1.0f);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindTexture(GL_TEXTURE_2D, texFloor); floorModel.draw();
        glBindTexture(GL_TEXTURE_2D, texWall); wallsModel.draw();
        glBindTexture(GL_TEXTURE_2D, texCeiling); ceilingModel.draw();

        window.display();
    }
    return 0;
}
