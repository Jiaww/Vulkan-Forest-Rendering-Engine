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
			Vertex vertex;
			// pos,normal,color ....
			vertex.pos.x = mesh->mVertices[i].x;
			vertex.pos.y = mesh->mVertices[i].z;
			vertex.pos.z = -mesh->mVertices[i].y;

			vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = mesh->mTextureCoords[0][i].y;

			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].z;
			vertex.normal.z = -mesh->mNormals[i].y;

			vertex.tangent.x = mesh->mTangents[i].x;
			vertex.tangent.y = mesh->mTangents[i].z;
			vertex.tangent.z = -mesh->mTangents[i].y;

			vertex.bitangent.x = mesh->mBitangents[i].x;
			vertex.bitangent.y = mesh->mBitangents[i].z;
			vertex.bitangent.z = -mesh->mBitangents[i].y;

			vertex.color.x = mesh->mColors[0][i].r;
			vertex.color.y = mesh->mColors[0][i].g;
			vertex.color.z = mesh->mColors[0][i].b;
			vertex.color.w = mesh->mColors[0][i].a;

			vertices.push_back(vertex);
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