#pragma once
#ifndef DATA_H
#define DATA_H
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

//半边数据结构定义多边形边界元素

class Vertex;
class HalfEdge;
class Loop;
class Face;
class Solid;
class BrepObject;

class FaceData {
public:
	int innerLoopNum, outerLoopIndex, solidIndex;
	std::vector<int> innerLoopIndexArray;
	FaceData() {
		innerLoopNum = outerLoopIndex = solidIndex = 0;
	}
};

class BrepObject {
public:
	Solid* solid;
	int vertexNum, loopNum, faceNum, solidNum;

	//拓扑关系数组
	std::vector<Vertex*> vertexList;
	std::vector<Loop*> loopList;
	std::vector<Face*> faceList;

	//GPU数据数组
	std::vector<float> vertexArray;
	std::vector<std::vector<int> > loopArray;
	std::vector<FaceData> faceArray;

	BrepObject() {
		solid = NULL;
		vertexNum = loopNum = faceNum = solidNum = 0;
	}

	void ListToArray();
	void PrintInfo();
};
BrepObject brep;

//点
class Vertex {
public:

	int id;
	glm::vec3 pos;

	Vertex(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f)) {
		id = 0;
		pos = position;
		brep.vertexList.emplace_back(this);
	}
};

//边(物理边)
class Edge {
public:
	int id;
	HalfEdge* he1, * he2;
	Edge* pre, * next;

	Edge() {
		id = 0;
		he1 = he2 = NULL;
		pre = next = NULL;
	}
};

//半边(逻辑边)
class HalfEdge {
public:
	int id;
	Vertex* startv, * endv;
	Edge* edge;
	Loop* loop;

	HalfEdge* pre, * next, * broHe;

	HalfEdge() {
		id = 0;
		startv = endv = NULL;
		edge = NULL;
		loop = NULL;
		pre = next = broHe = NULL;
	}
};

//环
//type为0表示外环，为1表示内环
class Loop {
public:

	int id, type;
	HalfEdge* he;
	Face* face;

	Loop* pre, * next;

	Loop() {
		type = 0;
		id = 0;
		he = NULL;
		face = NULL;
		pre = next = NULL;
		brep.loopList.emplace_back(this);
	}

	glm::vec3 CalcNormalDir() {
		Vertex* v1 = he->startv, * v2 = he->endv, * v3 = he->next->endv;
		return glm::normalize(glm::cross(v2->pos - v1->pos, v3->pos - v2->pos));
	}
};

//面
class Face {
public:

	int id;
	glm::vec3 normalDir;
	Loop* loop;
	Solid* solid;

	Face* pre, * next;

	std::vector<Loop*> innerLoopList;

	Face() {
		id = 0;
		loop = NULL;
		solid = NULL;
		pre = next = NULL;
		normalDir = glm::vec3(0);
		brep.faceList.emplace_back(this);
	}

	void CalcNormalDir() {
		normalDir = loop->CalcNormalDir();
	}

	void Erase() {
		//if(loop)	brep.loopArray.erase(find(brep.loopArray.begin(), brep.loopArray.end(), loop));
		brep.faceList.erase(find(brep.faceList.begin(), brep.faceList.end(), this));
	}
};

//三维实体
class Solid {
public:
	int id;
	Vertex* firstVertex;
	Edge* edgeList;
	Face* faceList;

	Solid* pre, * next;

	Solid() {
		id = 0;
		firstVertex = NULL;
		edgeList = NULL;
		faceList = NULL;
		pre = next = NULL;
	}
};

void BrepObject::ListToArray() {
	vertexNum = vertexList.size();
	for (int i = 0; i < vertexNum; i++) {
		vertexList[i]->id = i;
		vertexArray.emplace_back(vertexList[i]->pos.x);
		vertexArray.emplace_back(vertexList[i]->pos.y);
		vertexArray.emplace_back(vertexList[i]->pos.z);
	}
	loopNum = loopList.size();
	for (int i = 0; i < loopNum; i++) {
		loopList[i]->id = i;
		std::vector<int> temp(1, loopList[i]->he->startv->id);
		for (HalfEdge* he = loopList[i]->he; he->next != loopList[i]->he; he = he->next)
			temp.emplace_back(he->endv->id);
		loopArray.emplace_back(temp);
	}
	faceNum = faceList.size();
	for (int i = 0; i < faceNum; i++) {
		faceList[i]->id = i;
		FaceData temp;
		temp.outerLoopIndex = faceList[i]->loop->id;
		temp.innerLoopNum = faceList[i]->innerLoopList.size();
		for (int j = 0; j < temp.innerLoopNum; j++)
			temp.innerLoopIndexArray.emplace_back(faceList[i]->innerLoopList[j]->id);
		temp.solidIndex = 0;
		faceArray.emplace_back(temp);
	}
}


void BrepObject::PrintInfo() {
	std::cout << vertexNum << "个顶点数据:" << std::endl;
	for (int i = 0; i < vertexArray.size(); i++) {
		std::cout << vertexArray[i] << ' ';
		if ((i + 1) % 3 == 0)
			std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << loopNum << "个环数据:" << std::endl;
	for (int i = 0; i < loopArray.size(); i++) {
		std::cout << loopArray[i].size();
		for (int j = 0; j < loopArray[i].size(); j++)
			std::cout << ' ' << loopArray[i][j];
		std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << faceNum << "个面数据:" << std::endl;
	for (int i = 0; i < faceArray.size(); i++) {
		std::cout << faceArray[i].outerLoopIndex << ' ' << faceArray[i].innerLoopNum;
		for (int j = 0; j < faceArray[i].innerLoopIndexArray.size(); j++)
			std::cout << ' ' << faceArray[i].innerLoopIndexArray[j];
		std::cout << ' ' << faceArray[i].solidIndex << std::endl;
	}
}

#endif // !DATA_H