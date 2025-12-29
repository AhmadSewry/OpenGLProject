#include <SFML/Window/ContextSettings.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <vector>
#include <GL/glew.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/VideoMode.hpp>
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

// œ«·… „”«⁄œ… ·—”„ «·„” ÿÌ·« 
void addQuad(vector<vec3>& positions, vector<vec2>& uvs,
    vec3 p1, vec3 p2, vec3 p3, vec3 p4) {
    // «·„À·À «·√Ê·
    positions.push_back(p1);
    positions.push_back(p2);
    positions.push_back(p4);
    uvs.push_back({ 0.0f, 0.0f });
    uvs.push_back({ 1.0f, 0.0f });
    uvs.push_back({ 0.0f, 1.0f });
    // «·„À·À «·À«‰Ì
    positions.push_back(p2);
    positions.push_back(p3);
    positions.push_back(p4);
    uvs.push_back({ 1.0f, 0.0f });
    uvs.push_back({ 1.0f, 1.0f });
    uvs.push_back({ 0.0f, 1.0f });
}

GLuint generateTexture(const char* imagePath) {
    Image img;
    if (!img.loadFromFile(imagePath)) {
        cerr << "Failed to load texture: " << imagePath << endl;
        // Ì„ﬂ‰ ≈÷«›… ﬂÊœ ·≈‰‘«¡ texture «› —«÷Ì Â‰« · Ã‰» «·ﬂ—«‘
    }
    img.flipVertically();
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    return texture;
}

// œ«·…  Õ„Ì· «·„ÊœÌ·« 
Model loadModelFromFile(const char* meshPath) {
    Importer importer;
    const aiScene* scene = importer.ReadFile(meshPath, aiProcess_Triangulate);
    if (!scene || !scene->mMeshes[0]) {
        cerr << "Failed to load model: " << meshPath << endl;
        return Model({}, {});
    }
    aiMesh* mesh = scene->mMeshes[0];
    vector<vec3> positions;
    vector<vec2> uvs;
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        for (int j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
            auto position = mesh->mVertices[mesh->mFaces[i].mIndices[j]];
            vec3 pos = { position.x, position.y, position.z };
            positions.push_back(pos);
            if (mesh->mTextureCoords[0]) {
                auto uv = mesh->mTextureCoords[0][mesh->mFaces[i].mIndices[j]];
                uvs.push_back({ uv.x, uv.y });
            }
            else {
                uvs.push_back({ 0.0f, 0.0f });
            }
        }
    }
    return Model(positions, uvs);
}

int main() {
    cout << "testing ";
    ContextSettings ctxSettings;
    ctxSettings.minorVersion = 3;
    ctxSettings.majorVersion = 3;
    ctxSettings.depthBits = 24;

    Window window(VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Car Showroom - Full Room", Style::Default, State::Windowed, ctxSettings);
    window.setActive(true);

    if (glewInit() != GLEW_OK) return -1;
    glEnable(GL_DEPTH_TEST);

    float roomSize = 10.0f;
    float roomHeight = 6.0f;

    // --- 1.  ÃÂÌ“ «·√—÷Ì… (Floor) ---
    vector<vec3> floorPos;
    vector<vec2> floorUvs;
    addQuad(floorPos, floorUvs,
        { -roomSize, -2.0f,  roomSize },
        { roomSize, -2.0f,  roomSize },
        { roomSize, -2.0f, -roomSize },
        { -roomSize, -2.0f, -roomSize }
    );
    Model floorModel(floorPos, floorUvs);

    // --- 2.  ÃÂÌ“ «·Ãœ—«‰ (Walls) ---
    vector<vec3> wallPos;
    vector<vec2> wallUvs;

    // «·Œ·›Ì
    addQuad(wallPos, wallUvs,
        { -roomSize, -2.0f, -roomSize },
        { roomSize, -2.0f, -roomSize },
        { roomSize,  roomHeight, -roomSize },
        { -roomSize,  roomHeight, -roomSize }
    );
    // «·√Ì”—
    addQuad(wallPos, wallUvs,
        { -roomSize, -2.0f,  roomSize },
        { -roomSize, -2.0f, -roomSize },
        { -roomSize,  roomHeight, -roomSize },
        { -roomSize,  roomHeight,  roomSize }
    );
    // «·√Ì„‰
    addQuad(wallPos, wallUvs,
        { roomSize, -2.0f, -roomSize },
        { roomSize, -2.0f,  roomSize },
        { roomSize,  roomHeight,  roomSize },
        { roomSize,  roomHeight, -roomSize }
    );
    // «·√„«„Ì
    addQuad(wallPos, wallUvs,
        { -roomSize, -2.0f,  roomSize },
        { -roomSize,  roomHeight,  roomSize },
        { roomSize,  roomHeight,  roomSize },
        { roomSize, -2.0f,  roomSize }
    );
    Model wallsModel(wallPos, wallUvs);

    // --- 3.  ÃÂÌ“ «·”ﬁ› (Ceiling) [ÃœÌœ] ---
    vector<vec3> ceilingPos;
    vector<vec2> ceilingUvs;
    addQuad(ceilingPos, ceilingUvs,
        { -roomSize, roomHeight, -roomSize }, // √⁄·Ï Ì”«— Œ·›Ì
        { roomSize, roomHeight, -roomSize }, // √⁄·Ï Ì„Ì‰ Œ·›Ì
        { roomSize, roomHeight,  roomSize }, // √⁄·Ï Ì„Ì‰ √„«„Ì
        { -roomSize, roomHeight,  roomSize }  // √⁄·Ï Ì”«— √„«„Ì
    );
    Model ceilingModel(ceilingPos, ceilingUvs);


    // ---  Õ„Ì· «·’Ê— ---
    GLuint texFloor = generateTexture("floor.jpeg");   // √—÷Ì…
    GLuint texWall = generateTexture("wall.jpeg");     // Ãœ—«‰
    GLuint texCeiling = generateTexture("ceiling2.jpeg"); // ”ﬁ› ( √ﬂœ „‰ ÊÃÊœ «·’Ê—…)

    ShaderProgram shaderProgram("shaders/shader.vert", "shaders/shader.frag");
    GLuint modelMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "model");
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "view");
    GLuint perspectiveMatrixLocation = glGetUniformLocation(shaderProgram.getProgram(), "perspective");

    glUseProgram(shaderProgram.getProgram());

    vec3 cameraPosition{ 0.0f, 0.0f, 5.0f };
    vec3 cameraFront{ 0.0f, 0.0f, -1.0f };
    vec3 up{ 0.0f, 1.0f, 0.0f };

    mat4 perspectiveMatrix = perspective(45.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(perspectiveMatrixLocation, 1, GL_FALSE, &perspectiveMatrix[0][0]);

    Clock clock;
    bool running = true;

    while (running) {
        float deltaTime = clock.restart().asSeconds();
        float cameraSpeed = 5.0f * deltaTime;

        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) running = false;
            else if (const auto* resized = event->getIf<Event::Resized>())
                glViewport(0, 0, resized->size.x, resized->size.y);
        }

        // «· Õﬂ„
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

        // --- «·—”„ ---

        // 1. —”„ «·√—÷Ì…
        glBindTexture(GL_TEXTURE_2D, texFloor);
        floorModel.draw();

        // 2. —”„ «·Ãœ—«‰
        glBindTexture(GL_TEXTURE_2D, texWall);
        wallsModel.draw();

        // 3. —”„ «·”ﬁ› (ÃœÌœ)
        glBindTexture(GL_TEXTURE_2D, texCeiling);
        ceilingModel.draw();

        window.display();
    }
    return 0;
}
