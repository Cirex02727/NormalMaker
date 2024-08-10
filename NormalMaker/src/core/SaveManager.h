#pragma once

class SaveManager
{
public:
	static void Load();

	static void Unload();

	static YAML::Node GetNode(const std::string& nodeName);

private:
	static const std::string m_Filename;

	static YAML::Node m_Node;
};
