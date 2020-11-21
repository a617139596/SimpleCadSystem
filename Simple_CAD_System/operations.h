#ifndef OPERATION_H
#define OPERATION_H

#include "DataStructure.h"
#include <glm/glm.hpp>

template <class T>
void AddList(T* newObject, T* inListObject) {
	if (inListObject == NULL) {
		newObject->pre = newObject;
		newObject->next = newObject;
	}
	else if (inListObject != NULL && inListObject->next == NULL) {
		inListObject->next = newObject;
		newObject->pre = inListObject;
		inListObject->pre = newObject;
		newObject->next = inListObject;
	}
	else if (inListObject != NULL && inListObject->next != NULL) {
		newObject->next = inListObject->next;
		inListObject->next->pre = newObject;
		newObject->pre = inListObject;
		inListObject->next = newObject;
	}
}

void SwapFaceLoop(Loop* lp1, Loop* lp2) {
	Face* f1 = lp1->face, * f2 = lp2->face;
	lp1->face = f2;
	lp2->face = f1;
	if (f1)	f1->loop = lp2;
	if (f2)	f2->loop = lp1;
}

Face* FindFaceAtPoint(glm::vec3 pos, Face* f, Face* ignoreFace = NULL) {
	Face* initF = f;
	bool find = false;
	do {
		if (f == ignoreFace) {
			f = f->next;
			continue;
		}
		glm::vec3 p1 = pos - f->loop->he->startv->pos;
		glm::vec3 p2 = pos - f->loop->he->next->next->startv->pos;
		if (abs(glm::dot(f->normalDir, p1)) < 0.001 && abs(glm::dot(f->normalDir, p2)) < 0.001) {
			find = true;
			break;
		}
		f = f->next;
	} while (f != initF);
	return find ? f : NULL;
}

//假设选取传入的环都是满足有效性判断的环
//一条实边的两个半边属于不同loop，两个loop的半边相互指向
//定义内环为与所在面外环一致方向
Solid* mvfs(glm::vec3 position) {
	Solid* newSolid = new Solid();
	Face* newFace = new Face();
	Vertex* newVertex = new Vertex(position);
	Loop* newLoop = new Loop();

	newSolid->faceList = newFace;
	newFace->solid = newSolid;
	newFace->loop = newLoop;
	newLoop->face = newFace;
	newSolid->firstVertex = newVertex;

	return newSolid;
}

//生成一个新顶点v2，并在已知点v1和v2间构建半边，将v2加入loop
HalfEdge* mev(Vertex* v1, glm::vec3 position, Loop* lp) {
	Vertex* v2 = new Vertex(position);
	HalfEdge* he1 = new HalfEdge();
	HalfEdge* he2 = new HalfEdge();
	Edge* edge = new Edge();

	he1->startv = v1, he1->endv = v2, he1->next = he2, he1->broHe = he2;
	he2->startv = v2, he2->endv = v1; he2->pre = he1, he2->broHe = he1;
	he1->loop = he2->loop = lp;
	he1->edge = he2->edge = edge;
	edge->he1 = he1, edge->he2 = he2;

	//如果环所指向的边为NULL，说明目前只有一个顶点
	if (lp->he == NULL) {
		he2->next = he1;
		he1->pre = he2;
		lp->he = he1;
	}
	else {
		HalfEdge* he = lp->he;
		for (; he->next->startv != v1; he = he->next);
		he2->next = he->next;
		he->next->pre = he2;
		he->next = he1;
		he1->pre = he;
	}

	return he1;
}

//选取两点生成一条边，并且组成一个面
Face* mef(Vertex* v1, Vertex* v2, Loop* &loop) {
	HalfEdge* he1 = new HalfEdge();
	HalfEdge* he2 = new HalfEdge();
	Edge* edge = new Edge();
	Loop* newLoop = new Loop();
	Face* newFace = new Face();
	//将loop链接成环
	AddList(newLoop, loop);
	AddList(newFace, loop->face);
	
	//将逻辑半边与物理实边相联系
	he1->startv = v1, he1->endv = v2, he1->broHe = he2;
	he2->startv = v2, he2->endv = v1, he2->broHe = he1;
	edge->he1 = he1, edge->he2 = he2;
	he1->edge = he2->edge = edge;
	
	//连接新边，并进行环的分割
	auto he = loop->he;
	for (; he->next->startv != v1; he = he->next);
	he2->next = he->next;
	he->next->pre = he2;
	he->next = he1;
	he1->pre = he;
	for (; he->pre->endv != v2; he = he->pre);
	he->pre->next = he2;
	he2->pre = he->pre;
	he1->next = he;
	he->pre = he1;

	//修改边所属的loop，同时计算边数，将较小边数的环记录以便后续赋值给face
	int cnt1 = 0, cnt2 = 0;
	for (he = he1->next; he->loop != NULL; he = he->next, cnt1++);
	loop->he = he, he->loop = loop;
	for (he = he2->next; he->loop != NULL; he->loop = newLoop, he = he->next, cnt2++);
	he->loop = newLoop, newLoop->he = he;

	Face* tempFace = loop->face;
	//赋值给face较小环
	newFace->loop = cnt1 < cnt2 ? loop : newLoop;
	newFace->loop->face = newFace;
	newFace->CalcNormalDir();

	//修改newLoop使得loop和newLoop不指向同一环
	if (newFace->loop == loop) {
		newLoop->face = tempFace;
		loop = newLoop;
	}
	
	return newFace;
}

//删除一条边并形成一个内环
Loop* kemr(HalfEdge* he1, Loop* innerloop) {
	Vertex* v1 = he1->startv, * v2 = he1->endv;
	HalfEdge* he2 = he1->broHe, *temp1 = he1->pre, *temp2 = he1->next;
	Loop* newLoop = new Loop(), * lp = he1->loop;

	newLoop->type = 1;
	//令newLoop指向偏内的环
	newLoop->he = innerloop->he->broHe;

	he1->pre->next = he2->next;
	he2->next->pre = he1->pre;
	he1->next->pre = he2->pre;
	he2->pre->next = he1->next;
	
	//修改因为删除边而形成的两个环的偏内的环中边的信息
	newLoop->he->loop = newLoop;
	for (auto he = newLoop->he->next; he != newLoop->he; he = he->next)
		he->loop = newLoop;

	//保证偏外的环和偏内的环不指向同一边环
	lp->he = temp1->loop == newLoop ? temp2 : temp1;

	//将newLoop加入loop循环链表中
	AddList(newLoop, lp);
		
	//删除多余边
	delete(he1->edge);
	delete(he1);
	delete(he2);

	//定义内环为与所在面外环一致方向
	//交换环所属面
	if (lp->CalcNormalDir() == innerloop->CalcNormalDir()) {
		SwapFaceLoop(innerloop, newLoop);
		return innerloop;
	}
	return newLoop;
}

//删除一个面并且形成一个内环一个柄
Loop* kfmrh(Face* f) {
	//在循环链表中将f删除
	f->pre->next = f->next;
	f->next->pre = f->pre;

	//修改与f相关的数据
	Loop* lp = f->loop;
	lp->face = NULL;
	lp->type = 1;
	f->Erase();
	delete(f);

	return lp;
}

//扫成操作
//函数返回的是操作剩下的环
//面方向与扫成方向同向，扫成面指向体外，否则指向体内
//没有处理当内环扫成超限的问题
Loop* sweeping(Loop* loop, glm::vec3 vec, float d) {
	Face* adjFace = loop->he->broHe->loop->face;
	HalfEdge* newhe;
	Face* newFace;

	//待处理顶点(除首点)
	std::vector<Vertex*> vArr;
	for (HalfEdge* nexthe = loop->he; nexthe->next != loop->he; nexthe = nexthe->next)
		vArr.emplace_back(nexthe->endv);

	//先处理第一个点
	Vertex* prev, * newVertex;	
	newhe = mev(loop->he->startv, vec * d + loop->he->startv->pos, loop);
	newVertex = newhe->endv;
	prev = newVertex;
	
	//依次对后续点进行处理
	for (int i = 0; i < vArr.size(); i++) {
		newhe = mev(vArr[i], vec * d + vArr[i]->pos, loop);
		newFace = mef(prev, newhe->endv, loop);
		prev = newhe->endv;
	}
	newFace = mef(prev, newVertex, loop);

	Loop* tempLoop = adjFace->loop;
	SwapFaceLoop(loop, tempLoop);
	//由于交换了环重新计算法向
	adjFace->CalcNormalDir();

	return tempLoop;
}

#endif // !OPERATION_H
