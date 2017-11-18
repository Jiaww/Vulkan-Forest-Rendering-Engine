#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <Vertex.h>

class FbxLoader
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	FbxLoader(){}
	FbxLoader(const std::string path) {
		this->loadFbx(path);
	}
	// Fbx data info 
	std::vector<aiMesh*> meshes;
	std::string directory;

	void loadFbx(const std::string path);
	void processNode(aiNode* node, const aiScene* scene);
};