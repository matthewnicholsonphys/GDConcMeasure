#include "DataModel.h"

DataModel::DataModel(){}

TTree* DataModel::GetTTree(std::string name)
{
	return m_trees[name];
}


void DataModel::AddTTree(std::string name,TTree *tree)
{
	m_trees[name]=tree;
}


void DataModel::DeleteTTree(std::string name)
{
	m_trees.erase(name);
}

GdTree* DataModel::GetGdTree(std::string name)
{
	return m_gdtrees[name];
}


void DataModel::AddGdTree(std::string name, GdTree *tree)
{
	m_gdtrees[name]=tree;
}


void DataModel::DeleteGdTree(std::string name)
{
	m_gdtrees.erase(name);
}
