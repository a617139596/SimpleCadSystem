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
#include <poly2tri.h>

GLFWwindow* window;
extern BrepObject brep;

//�����
Camera camera(glm::vec3(0.0f, 0.0f, 20.0f));
float deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
float lastFrame = 0.0f; // ��һ֡��ʱ��
bool firstMouse = true;

//��Դ
glm::vec3 lightPos = glm::vec3(-20.0f, 40.0f, 20.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

//ʵ����ɫ
glm::vec3 outlineColor = glm::vec3(1.0f, 0.5f, 0.2f);
glm::vec3 faceColor = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 ringColor = glm::vec3(0.2f, 0.5f, 1.0f);

//���Ʊ���
int showMode = 1;
bool cullFace_switch = 0;

const float ScreenWidth = 800, ScreenHeight = 600;

void Init();
void Show();
void ModelDataCreate(const char filename[] = "Output.brp");
void ModelDataRead(const char filename[] = "breps.brp");
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void CameraMove();

int main(void) {
	int selectMode = 0;
	std::cout << "1������ģʽ������OperationInput.txt�ļ��ж�ȡ���ݴ���ʵ��ģ��\n2����ȡģʽ������ȡbreps.brp�ļ����ݽ�����ʾ\n����������ģʽ���:";
	std::cin >> selectMode;
	while (selectMode != 1 && selectMode != 2) {
		std::cout << "��Ч���룡" << std::endl;
		std::cin >> selectMode;
	}
	Init();
	switch (selectMode) {
	case 1:
		ModelDataCreate();
		break;
	case 2:
		ModelDataRead();
		break;
	default:
		break;
	}
	Show();
	return 0;
}

//��ȡ�������ݴ���ʵ�壬�������ݴ洢Ϊ.brp�ļ���Ĭ����ΪOutput.brp
void ModelDataCreate(const char filename[]) {
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
	int innerLoopVertexNum, inputInnerLoopNum;
	Vertex* firstInnerVertex;
	Face* innerLoopFace, * innerFace;
	Loop* remainLoop;

	//������ڻ�-ɨ�ɲ�������ÿ���ڻ�����������һ��ɨ�ɲ���
	brepFile >> inputInnerLoopNum;
	for (int k = 0; k < inputInnerLoopNum; k++) {
		//�ڶ����ڻ�
		brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
		//Ѱ���ڻ�������
		innerLoopFace = FindFaceAtPoint(position, solid->faceList);
		//���������Ͻ���mev��������֤�ڻ�����
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
		//���ɨ�ɺ���������غϣ����γ�ͨ��
		innerLoopFace = FindFaceAtPoint(innerFace->loop->he->startv->pos, solid->faceList, innerFace);
		if (innerLoopFace != NULL) {
			remainLoop = kfmrh(innerFace);
			innerLoopFace->innerLoopList.emplace_back(remainLoop);
		}
	}
		
	/*
	*/

	//����������תΪ��GPUʹ�õ���������
	brep.ListToArray();
	//��ӡ����
	brep.PrintInfo();
	brep.OutputData(filename);
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
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		showMode = 3;
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		cullFace_switch ^= 1;
		if (cullFace_switch)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
	}
}

//����ͷ�ƶ�
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
void ModelDataRead(const char filename[]) {
	std::ifstream brepFile(filename);
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
		//ȡ���������ķ���
		int loop_id = temp.outerLoopIndex;
		glm::vec3 pos[3];
		for (int j = 0; j < 3; j++) {
			int vertex_id = brep.loopArray[loop_id][j];
			pos[j] = glm::vec3(brep.vertexArray[3 * vertex_id], brep.vertexArray[3 * vertex_id + 1], brep.vertexArray[3 * vertex_id + 2]);
		}
		temp.normalDir = glm::normalize(glm::cross(pos[1] - pos[0], pos[2] - pos[1]));
		brep.faceArray.emplace_back(temp);
	}
	brepFile.close();
	brep.PrintInfo();
}

//������ʾģ��
void Show() {
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);

	//glClearColor(0.2f, 0.4f, 0.3f, 1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glLineWidth(3);

	//������񻯺������ζ������������
	std::vector<std::vector<float> > dataArray;
	for (int i = 0; i < brep.faceNum; i++) {
		int j = brep.faceArray[i].outerLoopIndex;
		//�����������ر���
		std::vector<p2t::Triangle*> triangles;
		std::vector<p2t::Point*> polyline;
		//�ж���ĳ�������ݽ��б任��ά
		//��ȡ�����׵��������任�ĳ�ʼ��
		int p = brep.loopArray[j][0];
		glm::vec3 pos1 = glm::vec3(brep.vertexArray[3 * p], brep.vertexArray[3 * p + 1], brep.vertexArray[3 * p + 2]);
		//��ƽ���任���ֲ�����ϵ�У�����ϵ����Ϊ�׵����꣬z��Ϊ����y���ɼ���ó�(�����ڷ���Ϊ(1, 1, 1)��������ܱ����˳���Ϊ����ֻ���м򵥿���)
		glm::vec3 front = brep.faceArray[i].normalDir;
		glm::mat4 trans = glm::lookAt(pos1, pos1 + front, glm::vec3(1.0f, 1.0f, 1.0f) - glm::vec3(abs(front.x), abs(front.y), abs(front.z)));
		//���⻷ÿ���������任���ֲ�����ϵ�£�����ά�����polyline������
		for (int k = 0; k < brep.loopArray[j].size(); k++) {
			p = brep.loopArray[j][k];
			glm::vec3 pos = glm::vec3(brep.vertexArray[3 * p], brep.vertexArray[3 * p + 1], brep.vertexArray[3 * p + 2]);
			//����ϵ�任
			glm::vec4 ans = trans * glm::vec4(pos, 1.0f);
			polyline.push_back(new p2t::Point(ans.x, ans.y));
		}
		p2t::CDT* cdt = new p2t::CDT(polyline);
		//���ڻ����в���
		if (brep.faceArray[i].innerLoopNum > 0) {
			for (int q = 0; q < brep.faceArray[i].innerLoopNum; q++) {
				std::vector<p2t::Point*> holeline;
				int innerLoopIndex = brep.faceArray[i].innerLoopIndexArray[q];
				for (int k = 0; k < brep.loopArray[innerLoopIndex].size(); k++) {
					p = brep.loopArray[innerLoopIndex][k];
					glm::vec3 pos = glm::vec3(brep.vertexArray[3 * p], brep.vertexArray[3 * p + 1], brep.vertexArray[3 * p + 2]);
					glm::vec4 ans = trans * glm::vec4(pos, 1.0f);
					holeline.push_back(new p2t::Point(ans.x, ans.y));
				}
				//��ӿ׶�
				cdt->AddHole(holeline);
			}
		}
		//���ж��������
		cdt->Triangulate();
		triangles = cdt->GetTriangles();

		//�ݴ������
		std::vector<float> tempArray;
		for (int k = 0; k < triangles.size(); k++) {
			p2t::Triangle& t = *triangles[k];
			for (int j = 0; j < 3; j++) {
				p2t::Point& a = *t.GetPoint(j);
				glm::vec4 ans = glm::vec4(a.x, a.y, 0.0f, 1.0f);
				//�����񻯺�ĵ㷴�任��ԭλ��
				ans = glm::inverse(trans) * ans;
				tempArray.emplace_back(ans.x);
				tempArray.emplace_back(ans.y);
				tempArray.emplace_back(ans.z);
				tempArray.emplace_back(brep.faceArray[i].normalDir.x);
				tempArray.emplace_back(brep.faceArray[i].normalDir.y);
				tempArray.emplace_back(brep.faceArray[i].normalDir.z);
			}
		}
		dataArray.emplace_back(tempArray);
	}

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	//���Ȱ󶨶�������VAO����������VBO��EBO�Ĳ������ᱻ�洢��VAO��
	glBindVertexArray(VAO);
	//����VBO������Ӧ�Ķ�������
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_STATIC_DRAW);
	//����VBO������Ӧ�Ķ�������
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//����GPU���������ʽ�Լ�ʹ�õĸ�ʽ
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (void*)0);
	//glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	Shader solidShader("vertexShader.vs", "fragmentShader.fs");
	solidShader.use();
	solidShader.setVec3("lightPos", lightPos);
	solidShader.setVec3("lightColor", lightColor);

	while (!glfwWindowShouldClose(window)) {
		CameraMove();
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float curTime = glfwGetTime();
		deltaTime = curTime - lastFrame;
		lastFrame = curTime;

		//�۲����
		glm::mat4 model = glm::mat4(1.0f);
		//ͶӰ����
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(camera.Zoom), ScreenWidth / ScreenHeight, 0.1f, 100.0f);

		solidShader.use();
		solidShader.setMat4("model", model);
		solidShader.setMat4("view", camera.GetViewMatrix());
		solidShader.setMat4("projection", projection);
		solidShader.setVec3("viewPos", camera.Position);
		solidShader.setBool("isFrameMode", true);

		glBindVertexArray(VAO);

		//���λ���������
		for (int i = 0, j; i < brep.faceNum; i++) {
			switch (showMode) {
			case 1:
				//������ʾ
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				solidShader.setVec3("color", faceColor);
				solidShader.setBool("isFrameMode", false);
				glBufferData(GL_ARRAY_BUFFER, dataArray[i].size() * sizeof(float), &dataArray[i][0], GL_DYNAMIC_DRAW);
				//����GPU���������ʽ�Լ�ʹ�õĸ�ʽ
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_TRIANGLES, 0, dataArray[i].size() / 6);
				break;
			case 2:
				//��������ʾ
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				solidShader.setVec3("color", outlineColor);
				glBufferData(GL_ARRAY_BUFFER, dataArray[i].size() * sizeof(float), &dataArray[i][0], GL_DYNAMIC_DRAW);
				//����GPU���������ʽ�Լ�ʹ�õĸ�ʽ
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_TRIANGLES, 0, dataArray[i].size() / 6);
				break;
			case 3:
				//���߿���ʾģʽ
				//��������
				glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_DYNAMIC_DRAW);
				//����GPU���������ʽ�Լ�ʹ�õĸ�ʽ
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
				//���⻷
				j = brep.faceArray[i].outerLoopIndex;
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				solidShader.setVec3("color", outlineColor);
				glDrawElements(GL_LINE_LOOP, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				//���ڻ�
				for (j = 0; j < brep.faceArray[i].innerLoopNum; j++) {
					int k = brep.faceArray[i].innerLoopIndexArray[j];
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[k].size() * sizeof(int), &brep.loopArray[k][0], GL_DYNAMIC_DRAW);
					solidShader.setVec3("color", ringColor);
					glDrawElements(GL_LINE_LOOP, brep.loopArray[k].size(), GL_UNSIGNED_INT, 0);
				}
				break;
			default:
				break;
			}
			
		}

		glBindVertexArray(0);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
}
