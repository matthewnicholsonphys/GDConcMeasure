#include "DataModel.h"

DataModel::DataModel(): CStore(false,0) {}


TTree* DataModel::GetTTree(std::string name){
  
  if(m_trees.count(name)>0) return m_trees[name];
  else return 0;
  
}


void DataModel::AddTTree(std::string name,TTree *tree){
  
  m_trees[name]=tree;

}


void DataModel::DeleteTTree(std::string name){
  
  m_trees.erase(name);
}

/*
GdTree* DataModel::GetGdTree(std::string name)
{
//	std::cout << "map size " << m_gdtrees.size() << std::endl;
//	std::cout << "GETTING from map " << name << "." << std::endl;
	if (m_gdtrees.count(name))
		return m_gdtrees[name];
	else
		return 0;
}


void DataModel::AddGdTree(std::string name, GdTree *tree)
{
	std::cout << "map size " << m_gdtrees.size() << std::endl;
	std::cout << "ADDING to map " << name << ",\t" << tree << "." << std::endl;
	m_gdtrees[name]=tree;
	std::cout << m_gdtrees[name] << std::endl;
}


void DataModel::DeleteGdTree(std::string name)
{
	std::cout << "map size " << m_gdtrees.size() << std::endl;
	std::cout << "REMOVING from map " << name << "." << std::endl;
	delete m_gdtrees[name];
	m_gdtrees.erase(name);
}

int DataModel::SizeGdTree()
{
	return m_gdtrees.size();
}
*/
