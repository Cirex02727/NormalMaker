#include "vkpch.h"
#include "SaveManager.h"

const std::string SaveManager::m_Filename = "saves.yml";

YAML::Node SaveManager::m_Node;

void SaveManager::Load()
{
	std::fstream file;
	file.open(m_Filename, std::fstream::in | std::fstream::out | std::fstream::app);
	file.close();

	m_Node = YAML::LoadFile(m_Filename);
}

void SaveManager::Unload()
{
	std::ofstream fout(m_Filename);
	fout << m_Node;
}

YAML::Node SaveManager::GetNode(const std::string& nodeName)
{
	return m_Node[nodeName];
}
