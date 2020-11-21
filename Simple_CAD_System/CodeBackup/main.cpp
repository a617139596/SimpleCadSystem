#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include "DataStructure.h"
#include "Shader.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "operations.h"

GLFWwindow* window;
extern BrepObject brep;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
float lastFrame = 0.0f; // ��һ֡��ʱ��
bool firstMouse = true;

glm::vec3 outlineColor = glm::vec3(1.0f, 0.5f, 0.2f);
glm::vec3 faceColor = glm::vec3(0.2f, 0.5f, 1.0f);

int showMode = 1;
bool cullFace_switch = 0;

const float ScreenWidth = 800, ScreenHeight = 600;

void Init();
void Show();
void ModelDataCreate();
void ModelDataRead();
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void CameraMove();

int main(void) {
	Init();
	ModelDataCreate();
	//ModelDataRead();
	Show();
	return 0;
}

void ModelDataCreate() {
	std::ifstream brepFile("OperationInput.txt");
	Solid* solid;
	glm::vec3 position;
	Vertex* lastVertex;
	HalfEdge* newhe;
	Face* newFace;
	int vertexNum;
	float sweepDis;	//ɨ�ɾ���

	//����һ����
	brepFile >> vertexNum;
	brepFile >> position.x >> position.y >> position.z;
	solid = mvfs(position);
	lastVertex = solid->firstVertex;
	for (int i = 1; i < vertexNum; i++) {
		brepFile >> position.x >> position.y >> position.z;
		newhe = mev(lastVertex, position, solid->faceList->loop);
		lastVertex = newhe->endv;
	}
	newFace = mef(lastVertex, solid->firstVertex, solid->faceList->loop);

	//ɨ�ɲ���
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(solid->faceList->loop, position, sweepDis);

	//�������Ѿ��������
	solid->faceList->CalcNormalDir();

	//�ڻ�
	int innerLoopVertexNum;
	Vertex* firstInnerVertex;
	brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
	//Ѱ���ڻ�������
	Face* innerLoopFace = FindFaceAtPoint(position, solid->faceList);
	//���������Ͻ���mev��������֤�ڻ�����
	newhe = mev(innerLoopFace->loop->he->startv, position, innerLoopFace->loop);
	firstInnerVertex = newhe->endv;
	lastVertex = firstInnerVertex;
	for (int i = 1; i < innerLoopVertexNum; i++) {
		brepFile >> position.x >> position.y >> position.z;
		newhe = mev(lastVertex, position, innerLoopFace->loop);
		lastVertex = newhe->endv;
	}
	Face* innerFace = mef(lastVertex, firstInnerVertex, innerLoopFace->loop);
	//kemr������Ҫ����ɨ�ɵĻ����������ڻ�ɨ���䷨��һ����ɨ�ɷ�����
	Loop* remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//����sweeping����ͨ������ʵ��������������߼�����ʵ���߼�����ƶ������innerFace�Ļ�ʵ������δ�����ڻ�
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//�ڻ�ɨ�ɲ���
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	//�ڶ����ڻ�
	brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
	innerLoopFace = FindFaceAtPoint(position, solid->faceList);
	newhe = mev(innerLoopFace->loop->he->startv, position, innerLoopFace->loop);
	firstInnerVertex = newhe->endv;
	lastVertex = firstInnerVertex;
	for (int i = 1; i < innerLoopVertexNum; i++) {
		brepFile >> position.x >> position.y >> position.z;
		newhe = mev(lastVertex, position, innerLoopFace->loop);
		lastVertex = newhe->endv;
	}
	innerFace = mef(lastVertex, firstInnerVertex, innerLoopFace->loop);

	//kemr������Ҫ����ɨ�ɵĻ����������ڻ�ɨ���䷨��һ����ɨ�ɷ�����
	remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//����sweeping����ͨ������ʵ��������������߼�����ʵ���߼�����ƶ������innerFace�Ļ�ʵ������δ�����ڻ�
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//�ڻ�ɨ�ɲ���
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	//�γ�ͨ��
	remainLoop = kfmrh(innerFace);
	innerLoopFace = FindFaceAtPoint(remainLoop->he->startv->pos, solid->faceList);
	innerLoopFace->innerLoopList.emplace_back(remainLoop);

	//�������ڻ�
	brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
	innerLoopFace = FindFaceAtPoint(position, solid->faceList);
	newhe = mev(innerLoopFace->loop->he->startv, position, innerLoopFace->loop);
	firstInnerVertex = newhe->endv;
	lastVertex = firstInnerVertex;
	for (int i = 1; i < innerLoopVertexNum; i++) {
		brepFile >> position.x >> position.y >> position.z;
		newhe = mev(lastVertex, position, innerLoopFace->loop);
		lastVertex = newhe->endv;
	}
	innerFace = mef(lastVertex, firstInnerVertex, innerLoopFace->loop);

	//kemr������Ҫ����ɨ�ɵĻ����������ڻ�ɨ���䷨��һ����ɨ�ɷ�����
	remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//����sweeping����ͨ������ʵ��������������߼�����ʵ���߼�����ƶ������innerFace�Ļ�ʵ������δ�����ڻ�
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//�ڻ�ɨ�ɲ���
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	/*
	*/

	//����������תΪ��GPUʹ�õ���������
	brep.ListToArray();
	//��ӡ����
	brep.PrintInfo();

}

//�ص����������ڴ�С�����ı�ʱ�ı���ͼ��С
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

//���ص�����������������벢����
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	static double lastX = ScreenWidth / 2, lastY = ScreenHeight / 2;
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
	lastX = xpos;
	lastY = ypos;
}

//���ռ������벢����
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		showMode = 1;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		showMode = 2;
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		cullFace_switch ^= 1;
		if (cullFace_switch)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
	}
}

void CameraMove() {
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

//��ʼ��GLFW��GLAD�����ûص�������������
void Init() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(ScreenWidth, ScreenHeight, "Simple_CAD_System", NULL, NULL);
	if (window == NULL) {
		std::cout << "Window Created Failed!" << std::endl;
		glfwTerminate();
		exit(-1);
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "GLAD Initiation Failed!" << std::endl;
		glfwTerminate();
		exit(-1);
	}
}

//Brep�ļ��Ķ�ȡ
void ModelDataRead() {
	std::ifstream brepFile("breps.brp");
	if (!brepFile.good()) {
		std::cout << "File Open Failed" << std::endl;
		exit(-1);
	}
	//���Ե�һ�е�"BRP"
	brepFile.ignore(4);
	brepFile >> brep.vertexNum >> brep.loopNum >> brep.faceNum >> brep.solidNum;
	//����δ����һ���ļ����solid�������ֻ���������
	for (int i = 0; i < brep.vertexNum * 3; i++) {
		float temp;
		brepFile >> temp;
		brep.vertexArray.emplace_back(temp);
	}
	for (int i = 0; i < brep.loopNum; i++) {
		int cnt;
		brepFile >> cnt;
		std::vector<int> tempArray(cnt);
		for (int j = 0; j < cnt; j++)
			brepFile >> tempArray[j];
		brep.loopArray.emplace_back(tempArray);
	}
	for (int i = 0; i < brep.faceNum; i++) {
		FaceData temp;
		brepFile >> temp.outerLoopIndex >> temp.innerLoopNum;
		for (int j = 0; j < temp.innerLoopNum; j++) {
			int tempInput;
			brepFile >> tempInput;
			temp.innerLoopIndexArray.emplace_back(tempInput);
		}
		brepFile >> temp.solidIndex;
		brep.faceArray.emplace_back(temp);
	}
	brepFile.close();
}

//������ʾģ��
void Show() {
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	//���Ȱ󶨶�������VAO����������VBO��EBO�Ĳ������ᱻ�洢��VAO��
	glBindVertexArray(VAO);
	//����VBO������Ӧ�Ķ�������
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_STATIC_DRAW);
	//����VBO������Ӧ�Ķ�������
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(brep.loopArray), &brep.loopArray[0], GL_STATIC_DRAW);
	//����GPU���������ʽ�Լ�ʹ�õĸ�ʽ
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	Shader myShader("vertexShader.vs", "fragmentShader.fs");
 

	while (!glfwWindowShouldClose(window)) {
		CameraMove();

		float curTime = glfwGetTime();
		deltaTime = curTime - lastFrame;
		lastFrame = curTime;

		//ͶӰ����
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(camera.Zoom), ScreenWidth / ScreenHeight, 0.1f, 100.0f);

		myShader.use();
		myShader.setMat4("view", camera.GetViewMatrix());
		myShader.setMat4("projection", projection);

		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glCullFace(GL_BACK);

		glClearColor(0.2f, 0.4f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLineWidth(3);

		glBindVertexArray(VAO);

		glFrontFace(GL_CW);//����֮ǰû�л��������棬�޷�������Ȳ��ԣ���Ҫ����������ڻ���(�ڻ��淽����)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		//ͨ�������ڻ�����ģ�建��
		glEnable(GL_BLEND);//�������
		glBlendEquation(GL_FUNC_ADD);//���û�Ϸ�ʽΪ����
		glBlendFunc(GL_ZERO, GL_ONE);//����������ǰ����ɫ���壬���������ڻ���
		glStencilMask(0xFF); // ����ģ�建��д��
		glStencilFunc(GL_ALWAYS, 1, 0xFF); // ���е�Ƭ�ζ�Ӧ�ø���ģ�建��
		for (int i = 0; i < brep.faceNum; i++) {
			for (int j = 0; j < brep.faceArray[i].innerLoopNum; j++) {
				int k = brep.faceArray[i].innerLoopIndexArray[j];
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[k].size() * sizeof(int), &brep.loopArray[k][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[k].size(), GL_UNSIGNED_INT, 0);
			}
		}
		glDisable(GL_BLEND);//�رջ��

		switch (showMode) {
		case 1:
			//�߿�ʵ����ʾģʽ
			//��һ�ν����⻷��Ļ���
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//�����⻷����һ��������ڣ���ʹ�����ڻ��汻�޳���һ���֣��γɵĶ���Ȳ�������δ���Ƶ�����У���˽�����Ȳ��Խ��޷���ȷ��ʾ�׶�
			glDisable(GL_STENCIL_TEST);
			glStencilMask(0x00); // ��ֹģ�建���д�룬ʵ�ִ��⻷����ȥ�ڻ����Ч��
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);//����ģ����Է�ʽ
			for (int i = 0; i < brep.faceNum; i++) {
				glFrontFace(GL_CCW);//������ʱ�뷽��Ϊ���򣬿ɽ��ڻ����޳�
				int j = brep.faceArray[i].outerLoopIndex;
				//����������ڻ�������ģ����ԣ�û���򲻿�������ֹ��������޷�����
				if (brep.faceArray[i].innerLoopNum != 0)
					glEnable(GL_STENCIL_TEST);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(faceColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				glDisable(GL_STENCIL_TEST);
			}

			//�ڶ��ν����߿�Ļ���
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//ʹ���߿�ģʽ����
			glDisable(GL_STENCIL_TEST);
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);//����ģ����Է�ʽ
			for (int i = 0; i < brep.faceNum; i++) {
				glFrontFace(GL_CCW);
				int j = brep.faceArray[i].outerLoopIndex;
				//����������ڻ�������ģ����ԣ�û���򲻿�������ֹ��������޷�����
				if (brep.faceArray[i].innerLoopNum != 0)
					glEnable(GL_STENCIL_TEST);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				//�����ڻ��߿�
				/*
				glFrontFace(GL_CW);
				for (int j = 0; j < brep.faceArray[i].innerLoopNum; j++) {
					int k = brep.faceArray[i].innerLoopIndexArray[j];
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[k].size() * sizeof(int), &brep.loopArray[k][0], GL_DYNAMIC_DRAW);
					myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
					glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[k].size(), GL_UNSIGNED_INT, 0);
				}
				*/
				glDisable(GL_STENCIL_TEST);
			}
			break;
		case 2:
			//ʵ�崿�߿���ʾģʽ
			for (int i = 0; i < brep.faceNum; i++) {
				int j = brep.faceArray[i].outerLoopIndex;
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
				glDrawElements(GL_LINE_LOOP, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
			}
			for (int i = 0; i < brep.faceNum; i++) {
				for (int j = 0; j < brep.faceArray[i].innerLoopNum; j++) {
					int k = brep.faceArray[i].innerLoopIndexArray[j];
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[k].size() * sizeof(int), &brep.loopArray[k][0], GL_DYNAMIC_DRAW);
					myShader.setVec4("lineColor", glm::vec4(faceColor, 1.0f));
					glDrawElements(GL_LINE_LOOP, brep.loopArray[k].size(), GL_UNSIGNED_INT, 0);
				}
			}
		default:
			break;
		}

		glStencilMask(0xFF);
		glBindVertexArray(0);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
}
