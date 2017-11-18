#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <string>
#include <vector>
#include <iostream>

class FbxLoader
{
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;
	std::vector<glm::vec4> colors;
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