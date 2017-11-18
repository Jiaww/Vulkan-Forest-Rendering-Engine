#include "FbxLoader.h"


void FbxLoader::loadFbx(const std::string path) {
	Assimp::Importer import;
	//import.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, 175);
	const aiScene* scene = import.ReadFile(path, aiProcess_OptimizeMeshes | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	import.ApplyPostProcessing(aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	this->directory = path.substr(0, path.find_last_of('/'));

	this->processNode(scene->mRootNode, scene);
}

void FbxLoader::processNode(aiNode* node, const aiScene* scene) {
	int offset = 0;
	// add all of meshes of current node
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		this->meshes.push_back(mesh);
		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			glm::vec3 vertex;
			// pos,normal,color ....
			vertex.x = mesh->mVertices[i].x;
			vertex.y = mesh->mVertices[i].z;
			vertex.z = -mesh->mVertices[i].y;
			vertices.push_back(vertex);
			glm::vec2 texCoord;
			texCoord.x = mesh->mTextureCoords[0][i].x;
			texCoord.y = mesh->mTextureCoords[0][i].y;
			texCoords.push_back(texCoord);
			glm::vec3 normal;
			normal.x = mesh->mNormals[i].x;
			normal.y = mesh->mNormals[i].z;
			normal.z = -mesh->mNormals[i].y;
			normals.push_back(normal);
			glm::vec3 tangent;
			tangent.x = mesh->mTangents[i].x;
			tangent.y = mesh->mTangents[i].z;
			tangent.z = -mesh->mTangents[i].y;
			tangents.push_back(tangent);
			glm::vec3 bitangent;
			bitangent.x = mesh->mBitangents[i].x;
			bitangent.y = mesh->mBitangents[i].z;
			bitangent.z = -mesh->mBitangents[i].y;
			bitangents.push_back(bitangent);
			glm::vec4 color;
			color.x = mesh->mColors[0][i].r;
			color.y = mesh->mColors[0][i].g;
			color.z = mesh->mColors[0][i].b;
			color.w = mesh->mColors[0][i].a;
			colors.push_back(color);
		}
		// indices
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				indices.push_back(offset + face.mIndices[j]);
		}
		offset = vertices.size();
	}
	// recursive call
	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		this->processNode(node->mChildren[i], scene);
	}
}