#pragma once

#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb/stb_image.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader/shader.h"
#include "texture/texture.h"

using namespace std;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 biTanget;
};

struct Material {
    glm::vec4 Ka;
    glm::vec4 Kd;
    glm::vec4 Ks;
    float Ns = 50.0f;
};

struct Mesh {
    GLuint VAO, VBO1, VBO2, VBO3, VBO4, VBO5, EBO;

    vector<glm::vec3> vertPositions;
    vector<glm::vec3> vertNormals;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> tangent;
    vector<glm::vec3> bitangent;
    vector<unsigned int> vertIndices;
    unsigned int texHandle;
    unsigned int normHandle;
    Material mats;
};

class Model {
    private:
        string modelDir, modelName;
        Assimp::Importer importer;
        const aiScene* scene = nullptr;
        aiNode* node = nullptr;

    public:
        unsigned int numMeshes;
        unsigned int numVertices;
        unsigned int numIndices;
        vector<Mesh> meshList;
        vector<Texture> textureList;

        Model(const char *modelPath) {
            // Path
            modelDir = string(modelPath);
            size_t lastSlash = modelDir.find_last_of("/\\");
            if (lastSlash != string::npos) {
                modelName = modelDir.substr(lastSlash + 1);
                modelDir = modelDir.substr(0, lastSlash);
            } else {
                modelDir = ".";
            }
            cout << "MODEL_DIR: " << modelDir << ", NAME: " << modelName << endl;

            // Texture class flips UV, so no need for aiProcess_FlipUVs
            scene = importer.ReadFile(modelPath, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
            loadModel();
        }

        void render(Shader& shader) const {
            for (unsigned int i = 0; i < numMeshes; i++) {
                bool hasTexture = meshList[i].texHandle > 0;
                bool hasNormalMap = meshList[i].normHandle > 0;
                shader.setInt("hasTex", hasTexture);
                shader.setInt("hasNormalMap", hasNormalMap);

                shader.setVec4("material.Ka", meshList[i].mats.Ka);
                shader.setVec4("material.Kd", meshList[i].mats.Kd);
                shader.setVec4("material.Ks", meshList[i].mats.Ks);
                shader.setFloat("material.Ns", meshList[i].mats.Ns);

                shader.setTexture("tex", meshList[i].texHandle, 0);
                shader.setTexture("normalMap", meshList[i].normHandle, 1);

                glBindVertexArray(meshList[i].VAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)meshList[i].vertIndices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

    private:
        void loadModel() {
            if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
                cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                return;
            }

            numMeshes = scene->mNumMeshes;
            meshList.resize(numMeshes);
            processNode();
        }

        void processNode() {
            aiMesh *mesh{};

            // Loop through all meshes
            for (unsigned int i = 0; i < numMeshes; ++i) {
                mesh = scene->mMeshes[i];
                aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                // Load textures
                loadMaterialTextures(material, aiTextureType_DIFFUSE, meshList[i].texHandle);
                loadMaterialTextures(material, aiTextureType_HEIGHT, meshList[i].normHandle);

                // Meshes
                // Vertices of mesh[i]
                for (unsigned int i2 = 0; i2 < mesh->mNumVertices; i2++) {
                    glm::vec3 position{};
                    position.x = mesh->mVertices[i2].x;
                    position.y = mesh->mVertices[i2].y;
                    position.z = mesh->mVertices[i2].z;
                    meshList[i].vertPositions.push_back(position);

                    // Normals
                    if (mesh->HasNormals()) {
                        glm::vec3 normal{};
                        normal.x = mesh->mNormals[i2].x;
                        normal.y = mesh->mNormals[i2].y;
                        normal.z = mesh->mNormals[i2].z;
                        meshList[i].vertNormals.push_back(normal);
                    } else
                        meshList[i].vertNormals.push_back(glm::vec3(0.0f));

                    // Texture Coordinates
                    // Only slot [0] is in question.
                    if (mesh->HasTextureCoords(0)) {
                        glm::vec2 texCoords{};
                        texCoords.x = mesh->mTextureCoords[0][i2].x;
                        texCoords.y = mesh->mTextureCoords[0][i2].y;
                        meshList[i].texCoords.push_back(texCoords);
                    } else
                        meshList[i].texCoords.push_back(glm::vec2(0.0f));

                    
                    // Tangents
                    /*
                    glm::vec3 tangent{};
                    tangent.x = mesh->mTangents[i2].x;
                    tangent.y = mesh->mTangents[i2].y;
                    tangent.z = mesh->mTangents[i2].z;
                    meshList[i].tangent.push_back(tangent);

                    // Bitangents
                    glm::vec3 bitangent{};
                    bitangent.x = mesh->mBitangents[i2].x;
                    bitangent.y = mesh->mBitangents[i2].y;
                    bitangent.z = mesh->mBitangents[i2].z;
                    meshList[i].bitangent.push_back(bitangent);
                    */
                    

                    numVertices++;
                }
                // Indices of mesh[i]
                for (unsigned int i3 = 0; i3 < mesh->mNumFaces; i3++) {
                    aiFace face = mesh->mFaces[i3];
                    for (unsigned int i4 = 0; i4 < face.mNumIndices; i4++) {
                        meshList[i].vertIndices.push_back(face.mIndices[i4]);
                        numIndices++;
                    }
                }

                // Materials
                aiMaterial* materials = scene->mMaterials[mesh->mMaterialIndex];
                aiColor3D color;
                materials->Get(AI_MATKEY_COLOR_AMBIENT, color);
                meshList[i].mats.Ka = glm::vec4(color.r, color.g, color.b, 1.0);
                materials->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                meshList[i].mats.Kd = glm::vec4(color.r, color.g, color.b, 1.0);
                materials->Get(AI_MATKEY_COLOR_SPECULAR, color);
                meshList[i].mats.Ks = glm::vec4(color.r, color.g, color.b, 1.0);
                material->Get(AI_MATKEY_SHININESS, meshList[i].mats.Ns);

                setBufferData(i);
                // cout << "   Vertices: " << numVertices << endl;
                // cout << "   Indices: " << numIndices << endl;
            }
        }

        void setBufferData(unsigned int index) {
            glGenVertexArrays(1, &meshList[index].VAO);
            glGenBuffers(1, &meshList[index].VBO1); // Alternative to using 3 separate VBOs, instead use only 1 VBO and set glVertexAttribPointer's offset...
            glGenBuffers(1, &meshList[index].VBO2); // like was done in tutorial 3... Orbiting spinning cubes.
            glGenBuffers(1, &meshList[index].VBO3);
            glGenBuffers(1, &meshList[index].EBO);

            glBindVertexArray(meshList[index].VAO);

            // Vertex Positions
            glBindBuffer(GL_ARRAY_BUFFER, meshList[index].VBO1);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * meshList[index].vertPositions.size(), meshList[index].vertPositions.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0); // Void pointer below is for legacy reasons. Two possible meanings: "offset for buffer objects" & "address for client state arrays"
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

            // Vertex Normals
            glBindBuffer(GL_ARRAY_BUFFER, meshList[index].VBO2);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * meshList[index].vertNormals.size(), meshList[index].vertNormals.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

            // Texture Coordinates
            glBindBuffer(GL_ARRAY_BUFFER, meshList[index].VBO3);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * meshList[index].texCoords.size(), meshList[index].texCoords.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

            /*
            // Tangents
            glBindBuffer(GL_ARRAY_BUFFER, meshList[index].VBO4);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * meshList[index].tangent.size(), meshList[index].tangent.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            // Bitangents
            glBindBuffer(GL_ARRAY_BUFFER, meshList[index].VBO5);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * meshList[index].bitangent.size(), meshList[index].bitangent.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            */

            // Indices for: glDrawElements()
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshList[index].EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * meshList[index].vertIndices.size(), &meshList[index].vertIndices[0], GL_STATIC_DRAW);

            // Unbind VAO, VBO, EBO
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        int isImageLoaded(string file_name) {
            for (unsigned int i = 0; i < textureList.size(); i++)
                if (file_name.compare(textureList[i].imageName) == 0)
                    return textureList[i].ID;
            return -1;
        }

        void loadMaterialTextures(aiMaterial* mat, aiTextureType type, unsigned int& textureHandle) {
            vector<Texture> textures;
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString str;
                mat->GetTexture(type, i, &str);

                string imageName = str.C_Str();
                int loaded = isImageLoaded(imageName);

                if (loaded == -1) {
                    string pathStr = modelDir + "/" + imageName;
                    const char* path = pathStr.c_str();
                    Texture tex(path);

                    textureList.push_back(tex);
                    textureHandle = tex.ID;
                } else
                    textureHandle = loaded;
            }
        }
};