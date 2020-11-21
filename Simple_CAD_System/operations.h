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

//����ѡȡ����Ļ�����������Ч���жϵĻ�
//һ��ʵ�ߵ�����������ڲ�ͬloop������loop�İ���໥ָ��
//�����ڻ�Ϊ���������⻷һ�·���
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

//����һ���¶���v2��������֪��v1��v2�乹����ߣ���v2����loop
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

	//�������ָ��ı�ΪNULL��˵��Ŀǰֻ��һ������
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

//ѡȡ��������һ���ߣ��������һ����
Face* mef(Vertex* v1, Vertex* v2, Loop* &loop) {
	HalfEdge* he1 = new HalfEdge();
	HalfEdge* he2 = new HalfEdge();
	Edge* edge = new Edge();
	Loop* newLoop = new Loop();
	Face* newFace = new Face();
	//��loop���ӳɻ�
	AddList(newLoop, loop);
	AddList(newFace, loop->face);
	
	//���߼����������ʵ������ϵ
	he1->startv = v1, he1->endv = v2, he1->broHe = he2;
	he2->startv = v2, he2->endv = v1, he2->broHe = he1;
	edge->he1 = he1, edge->he2 = he2;
	he1->edge = he2->edge = edge;
	
	//�����±ߣ������л��ķָ�
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

	//�޸ı�������loop��ͬʱ�������������С�����Ļ���¼�Ա������ֵ��face
	int cnt1 = 0, cnt2 = 0;
	for (he = he1->next; he->loop != NULL; he = he->next, cnt1++);
	loop->he = he, he->loop = loop;
	for (he = he2->next; he->loop != NULL; he->loop = newLoop, he = he->next, cnt2++);
	he->loop = newLoop, newLoop->he = he;

	Face* tempFace = loop->face;
	//��ֵ��face��С��
	newFace->loop = cnt1 < cnt2 ? loop : newLoop;
	newFace->loop->face = newFace;
	newFace->CalcNormalDir();

	//�޸�newLoopʹ��loop��newLoop��ָ��ͬһ��
	if (newFace->loop == loop) {
		newLoop->face = tempFace;
		loop = newLoop;
	}
	
	return newFace;
}

//ɾ��һ���߲��γ�һ���ڻ�
Loop* kemr(HalfEdge* he1, Loop* innerloop) {
	Vertex* v1 = he1->startv, * v2 = he1->endv;
	HalfEdge* he2 = he1->broHe, *temp1 = he1->pre, *temp2 = he1->next;
	Loop* newLoop = new Loop(), * lp = he1->loop;

	newLoop->type = 1;
	//��newLoopָ��ƫ�ڵĻ�
	newLoop->he = innerloop->he->broHe;

	he1->pre->next = he2->next;
	he2->next->pre = he1->pre;
	he1->next->pre = he2->pre;
	he2->pre->next = he1->next;
	
	//�޸���Ϊɾ���߶��γɵ���������ƫ�ڵĻ��бߵ���Ϣ
	newLoop->he->loop = newLoop;
	for (auto he = newLoop->he->next; he != newLoop->he; he = he->next)
		he->loop = newLoop;

	//��֤ƫ��Ļ���ƫ�ڵĻ���ָ��ͬһ�߻�
	lp->he = temp1->loop == newLoop ? temp2 : temp1;

	//��newLoop����loopѭ��������
	AddList(newLoop, lp);
		
	//ɾ�������
	delete(he1->edge);
	delete(he1);
	delete(he2);

	//�����ڻ�Ϊ���������⻷һ�·���
	//������������
	if (lp->CalcNormalDir() == innerloop->CalcNormalDir()) {
		SwapFaceLoop(innerloop, newLoop);
		return innerloop;
	}
	return newLoop;
}

//ɾ��һ���沢���γ�һ���ڻ�һ����
Loop* kfmrh(Face* f) {
	//��ѭ�������н�fɾ��
	f->pre->next = f->next;
	f->next->pre = f->pre;

	//�޸���f��ص�����
	Loop* lp = f->loop;
	lp->face = NULL;
	lp->type = 1;
	f->Erase();
	delete(f);

	return lp;
}

//ɨ�ɲ���
//�������ص��ǲ���ʣ�µĻ�
//�淽����ɨ�ɷ���ͬ��ɨ����ָ�����⣬����ָ������
//û�д����ڻ�ɨ�ɳ��޵�����
Loop* sweeping(Loop* loop, glm::vec3 vec, float d) {
	Face* adjFace = loop->he->broHe->loop->face;
	HalfEdge* newhe;
	Face* newFace;

	//��������(���׵�)
	std::vector<Vertex*> vArr;
	for (HalfEdge* nexthe = loop->he; nexthe->next != loop->he; nexthe = nexthe->next)
		vArr.emplace_back(nexthe->endv);

	//�ȴ����һ����
	Vertex* prev, * newVertex;	
	newhe = mev(loop->he->startv, vec * d + loop->he->startv->pos, loop);
	newVertex = newhe->endv;
	prev = newVertex;
	
	//���ζԺ�������д���
	for (int i = 0; i < vArr.size(); i++) {
		newhe = mev(vArr[i], vec * d + vArr[i]->pos, loop);
		newFace = mef(prev, newhe->endv, loop);
		prev = newhe->endv;
	}
	newFace = mef(prev, newVertex, loop);

	Loop* tempLoop = adjFace->loop;
	SwapFaceLoop(loop, tempLoop);
	//���ڽ����˻����¼��㷨��
	adjFace->CalcNormalDir();

	return tempLoop;
}

#endif // !OPERATION_H
