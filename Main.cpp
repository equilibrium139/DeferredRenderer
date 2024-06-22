#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <tiny_gltf.h>
#include "Camera.h"
#include "Framebuffer.h"
#include "GLTFResources.h"
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

    window = glfwCreateWindow(windowWidth, windowHeight, "Simple example", NULL, NULL);
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
    Shader renderGBufferShader = Shader("Shaders/fullscreen.vert", "Shaders/fullscreen.frag");


    // TODO: create framebuffer struct to make creation simpler
    // Create G-buffer

    Framebuffer gBuffer{ windowWidth, windowHeight,
        {
            ColorAttachmentInfo {
                .internalFormat = GL_RGBA16F,
                .format = GL_RGBA,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo {
                .internalFormat = GL_RGB16F,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo {
                .internalFormat = GL_RGB16F,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo {
                .internalFormat = GL_RGB16F,
                .format = GL_RGB,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
            ColorAttachmentInfo {
                .internalFormat = GL_RGBA16F,
                .format = GL_RGBA,
                .type = GL_FLOAT,
                .minFilterMode = GL_LINEAR,
                .magFilterMode = GL_LINEAR
            },
        },
        GL_DEPTH24_STENCIL8
    };

    GLuint depthStencilRBO;
    glGenRenderbuffers(1, &depthStencilRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRBO);

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

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

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
        gBuffer.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        glm::mat4 worldView = view * duckWorldMat;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldView)));

        geometryPassShader.use();
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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        renderGBufferShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.colorTextures[1]);
        glBindVertexArray(fullscreenQuadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}
