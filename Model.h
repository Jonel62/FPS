#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h" 

class Model {
public:
    Model(const std::string& path, GLuint* t_diffuse, GLuint* t_specular) {
        if (t_diffuse != nullptr) {
            this->t_diffuse = *t_diffuse;
        }
        else {
            this->t_diffuse = 0;
        }
        if (t_specular != nullptr) {
            this->t_specular = *t_specular;
        }
        else {
            this->t_specular = 0;
        }
        loadModel(path);
    }

    void Draw(unsigned int shaderProgramID) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shaderProgramID);
        }
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    GLuint t_diffuse;
    GLuint t_specular;


    Assimp::Importer importer;

    void loadModel(const std::string& path) {
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenSmoothNormals
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "B£¥D ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));


        processNode(scene->mRootNode, scene);
    }


    void processNode(aiNode* node, const aiScene* scene) {

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }


    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;


            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;


            if (mesh->HasNormals()) {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }
            else {
                vertex.Normal = glm::vec3(0.0f, 0.0f, 0.0f);
            }


            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }


        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        Texture diffuse;
        diffuse.id = t_diffuse;
        diffuse.type = "texture_diffuse";
        textures.push_back(diffuse);

        Texture specular;
        specular.id = t_specular;
        specular.type = "texture_specular";
        textures.push_back(specular);
        return Mesh(vertices, indices, textures);
    }
};