#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <tiny_gltf.h>
#include "Camera.h"
#include "Framebuffer.h"
#include "GLTFResources.h"
#include "Light.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

const int windowWidth = 640;
const int windowHeight = 480;

static std::vector<std::string> GetShaderDefines(VertexAttribute flags, bool flatShading)
{
    std::vector<std::string> defines;

    if (HasFlag(flags, VertexAttribute::TEXCOORD))
    {
        defines.emplace_back("HAS_TEXCOORD");
    }
    if (HasFlag(flags, VertexAttribute::NORMAL))
    {
        defines.emplace_back("HAS_NORMALS");
    }
    if (HasFlag(flags, VertexAttribute::JOINTS))
    {
        defines.emplace_back("HAS_JOINTS");
    }
    if (HasFlag(flags, VertexAttribute::MORPH_TARGET0_POSITION))
    {
        defines.emplace_back("HAS_MORPH_TARGETS");
    }
    if (flatShading)
    {
        defines.emplace_back("FLAT_SHADING");
    }
    if (HasFlag(flags, VertexAttribute::TANGENT))
    {
        defines.emplace_back("HAS_TANGENTS");
    }
    if (HasFlag(flags, VertexAttribute::COLOR))
    {
        defines.emplace_back("HAS_VERTEX_COLORS");
    }

    return defines;
}

int main(void)
{
    GLFWwindow* window;

    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(windowWidth, windowHeight, "Deferred Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::string filepath = "C:\\dev\\gltf-models\\Duck\\glTF\\Duck.gltf";

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return -1;
    }
    Mesh duckMesh{ model.meshes[0], model };
    Shader geometryPassShader = Shader("Shaders/geometryPass.vert", "Shaders/geometryPass.frag", nullptr, GetShaderDefines(duckMesh.submeshes[0].flags, duckMesh.submeshes[0].flatShading));

    // All color attachments are used for the geometry pass except for the last attachment which is an HDR texture used in the lighting pass.
    // This makes it easy to use the depth buffer from the geometry pass in the lighting pass. 
    Framebuffer framebuffer{ windowWidth, windowHeight,
        {
            ColorAttachmentInfo { // base color
                .internalFormat = GL_RGBA8,
                .format = GL_RGBA,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo { // metallic roughness occlusion
                .internalFormat = GL_RGB8,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo { // normal
                .internalFormat = GL_RGB8,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo { // position
                .internalFormat = GL_RGB16F,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo { // hdr texture for lighting pass
                .internalFormat = GL_RGB16F,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
        },
        GL_DEPTH24_STENCIL8
    };
    framebuffer.Bind();
    glEnablei(GL_BLEND, framebuffer.colorTextures.size() - 1); 
    glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
    glBlendEquation(GL_FUNC_ADD);

    DirectionalLight light{ glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), 10.0f };
    Shader renderGBufferShader = Shader("Shaders/fullscreen.vert", "Shaders/fullscreen.frag");
    Shader lightingPassShader = Shader("Shaders/lighting.vert", "Shaders/lightingDirectional.frag");

    Shader postprocessShader = Shader("Shaders/fullscreen.vert", "Shaders/postprocess.frag");

    GLfloat dirLightVertices[] = {
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f
    };

    GLuint dirLightIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint dirLightVAO;
    glGenVertexArrays(1, &dirLightVAO);
    glBindVertexArray(dirLightVAO);

    GLuint dirLightVBO;
    glGenBuffers(1, &dirLightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, dirLightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dirLightVertices), dirLightVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    GLuint dirLightIBO;
    glGenBuffers(1, &dirLightIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dirLightIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(dirLightIndices), dirLightIndices, GL_STATIC_DRAW);

    GLfloat quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    GLuint quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint fullscreenQuadVAO;
    glGenVertexArrays(1, &fullscreenQuadVAO);
    glBindVertexArray(fullscreenQuadVAO);

    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);

    GLuint quadIBO;
    glGenBuffers(1, &quadIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);


    Camera camera;
    camera.position.y = 80.0f;
    camera.position.z = 210.0f;

    glm::mat4 duckWorldMat = glm::mat4(1.0f);
    //duckWorldMat = glm::rotate(duckWorldMat, -45.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    GLTFResources resources{ model };

    while (!glfwWindowShouldClose(window))
    {
        // Update

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render

        glEnable(GL_DEPTH_TEST);
        framebuffer.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        glm::mat4 worldView = view * duckWorldMat;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldView)));

        geometryPassShader.Use();
        geometryPassShader.SetMat4("world", duckWorldMat);
        geometryPassShader.SetMat4("view", view);
        geometryPassShader.SetMat4("projection", projection);
        geometryPassShader.SetMat3("normalMatrixVS", normalMatrix);

        Submesh& mesh = duckMesh.submeshes[0];
        Texture white = Texture::White1x1TextureRGBA();
        Texture red = Texture::Max1x1TextureRed();
        PBRMaterial& material = resources.materials[mesh.materialIndex];

        geometryPassShader.SetVec4("material.baseColorFactor", material.baseColorFactor);
        geometryPassShader.SetFloat("material.metallicFactor", material.metallicFactor);
        geometryPassShader.SetFloat("material.roughnessFactor", material.roughnessFactor);
        geometryPassShader.SetFloat("material.occlusionStrength", material.occlusionStrength);

        bool hasTextureCoords = HasFlag(mesh.flags, VertexAttribute::TEXCOORD);
        if (hasTextureCoords)
        {
            int textureUnit = 0;

            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, resources.textures[material.baseColorTextureIdx].id);
            geometryPassShader.SetInt("material.baseColorTexture", textureUnit);
            textureUnit++;

            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, resources.textures[material.metallicRoughnessTextureIdx].id);
            geometryPassShader.SetInt("material.metallicRoughnessTexture", textureUnit);
            textureUnit++;

            if (material.normalTextureIdx >= 0)
            {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, resources.textures[material.normalTextureIdx].id);
                geometryPassShader.SetInt("material.normalTexture", textureUnit);
                textureUnit++;

                // This uniform variable is only used if tangents (which are synonymous with normal mapping for now)
                // are provided
                if (HasFlag(mesh.flags, VertexAttribute::TANGENT))
                {
                    geometryPassShader.SetFloat("material.normalScale", material.normalScale);
                }
            }

            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, resources.textures[material.occlusionTextureIdx].id);
            geometryPassShader.SetInt("material.occlusionTexture", textureUnit);
            textureUnit++;
        }
        glBindVertexArray(mesh.VAO);
        if (mesh.hasIndexBuffer)
        {
            glDrawElements(GL_TRIANGLES, mesh.countVerticesOrIndices, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, mesh.countVerticesOrIndices);
        }


        lightingPassShader.Use();

        glm::mat4 dirLightMVP = glm::mat4(1.0f);
        lightingPassShader.SetMat4("mvp", dirLightMVP);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorTextures[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorTextures[1]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorTextures[2]);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorTextures[3]);

        lightingPassShader.SetInt("gBaseColor", 0);
        lightingPassShader.SetInt("gMetallicRoughnessOcclusion", 1);
        lightingPassShader.SetInt("gNormal", 2);
        lightingPassShader.SetInt("gPosition", 3);

        lightingPassShader.SetVec2("resolution", (float)windowWidth, (float)windowHeight);
        lightingPassShader.SetVec3("light.color", light.color);
        lightingPassShader.SetFloat("light.intensity", light.intensity);
        lightingPassShader.SetVec3("light.direction", light.directionVS);

        glBindVertexArray(dirLightVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);
        postprocessShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorTextures[4]);
        postprocessShader.SetInt("sceneColorsTexture", 0);
        postprocessShader.SetFloat("exposure", 1.0f);
        glBindVertexArray(fullscreenQuadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}
