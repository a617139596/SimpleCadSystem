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
float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间
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
	float sweepDis;	//扫成距离

	//建立一个面
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

	//扫成操作
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(solid->faceList->loop, position, sweepDis);

	//外侧基本已经建立完毕
	solid->faceList->CalcNormalDir();

	//内环
	int innerLoopVertexNum;
	Vertex* firstInnerVertex;
	brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
	//寻找内环所在面
	Face* innerLoopFace = FindFaceAtPoint(position, solid->faceList);
	//在所在面上进行mev操作，保证内环方向
	newhe = mev(innerLoopFace->loop->he->startv, position, innerLoopFace->loop);
	firstInnerVertex = newhe->endv;
	lastVertex = firstInnerVertex;
	for (int i = 1; i < innerLoopVertexNum; i++) {
		brepFile >> position.x >> position.y >> position.z;
		newhe = mev(lastVertex, position, innerLoopFace->loop);
		lastVertex = newhe->endv;
	}
	Face* innerFace = mef(lastVertex, firstInnerVertex, innerLoopFace->loop);
	//kemr返回需要进行扫成的环，由于是内环扫成其法向一定与扫成方向反向
	Loop* remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//由于sweeping操作通过交换实际面的两个环的逻辑面来实现逻辑面的移动，因此innerFace的环实际上是未来的内环
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//内环扫成操作
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	//第二个内环
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

	//kemr返回需要进行扫成的环，由于是内环扫成其法向一定与扫成方向反向
	remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//由于sweeping操作通过交换实际面的两个环的逻辑面来实现逻辑面的移动，因此innerFace的环实际上是未来的内环
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//内环扫成操作
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	//形成通孔
	remainLoop = kfmrh(innerFace);
	innerLoopFace = FindFaceAtPoint(remainLoop->he->startv->pos, solid->faceList);
	innerLoopFace->innerLoopList.emplace_back(remainLoop);

	//第三个内环
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

	//kemr返回需要进行扫成的环，由于是内环扫成其法向一定与扫成方向反向
	remainLoop = kemr(newhe->loop == innerFace->loop ? newhe->next->broHe->pre : newhe->next->next, innerFace->loop);
	//由于sweeping操作通过交换实际面的两个环的逻辑面来实现逻辑面的移动，因此innerFace的环实际上是未来的内环
	innerLoopFace->innerLoopList.emplace_back(remainLoop->he->broHe->loop);

	//内环扫成操作
	brepFile >> sweepDis;
	brepFile >> position.x >> position.y >> position.z;
	sweeping(remainLoop, position, sweepDis);

	/*
	*/

	//将拓扑数据转为给GPU使用的数组数据
	brep.ListToArray();
	//打印数据
	brep.PrintInfo();

}

//回调函数当窗口大小发生改变时改变视图大小
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

//鼠标回调函数，接收鼠标输入并处理
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

//接收键盘输入并处理
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

//初始化GLFW和GLAD，设置回调函数处理输入
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

//Brep文件的读取
void ModelDataRead() {
	std::ifstream brepFile("breps.brp");
	if (!brepFile.good()) {
		std::cout << "File Open Failed" << std::endl;
		exit(-1);
	}
	//忽略第一行的"BRP"
	brepFile.ignore(4);
	brepFile >> brep.vertexNum >> brep.loopNum >> brep.faceNum >> brep.solidNum;
	//暂且未考虑一个文件多个solid的情况，只是整体绘制
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

//定义显示模块
void Show() {
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	//事先绑定顶点数组VAO，这样所有VBO和EBO的操作都会被存储至VAO中
	glBindVertexArray(VAO);
	//设置VBO与其相应的顶点数据
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_STATIC_DRAW);
	//设置VBO与其相应的顶点数据
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(brep.loopArray), &brep.loopArray[0], GL_STATIC_DRAW);
	//设置GPU顶点解析格式以及使用的格式
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	Shader myShader("vertexShader.vs", "fragmentShader.fs");
 

	while (!glfwWindowShouldClose(window)) {
		CameraMove();

		float curTime = glfwGetTime();
		deltaTime = curTime - lastFrame;
		lastFrame = curTime;

		//投影矩阵
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

		glFrontFace(GL_CW);//由于之前没有绘制其他面，无法进行深度测试，需要剪除背面的内环面(内环面方向反向)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		//通过绘制内环面获得模板缓存
		glEnable(GL_BLEND);//开启混合
		glBlendEquation(GL_FUNC_ADD);//设置混合方式为叠加
		glBlendFunc(GL_ZERO, GL_ONE);//仅保留绘制前的颜色缓冲，即不绘制内环面
		glStencilMask(0xFF); // 启用模板缓冲写入
		glStencilFunc(GL_ALWAYS, 1, 0xFF); // 所有的片段都应该更新模板缓冲
		for (int i = 0; i < brep.faceNum; i++) {
			for (int j = 0; j < brep.faceArray[i].innerLoopNum; j++) {
				int k = brep.faceArray[i].innerLoopIndexArray[j];
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[k].size() * sizeof(int), &brep.loopArray[k][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[k].size(), GL_UNSIGNED_INT, 0);
			}
		}
		glDisable(GL_BLEND);//关闭混合

		switch (showMode) {
		case 1:
			//线框实体显示模式
			//第一次进行外环面的绘制
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//由于外环面以一个整体存在，即使由于内环面被剔除了一部分，形成的洞深度测试仍以未绘制的面进行，因此进行深度测试将无法正确显示孔洞
			glDisable(GL_STENCIL_TEST);
			glStencilMask(0x00); // 禁止模板缓冲的写入，实现从外环面挖去内环面的效果
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);//设置模板测试方式
			for (int i = 0; i < brep.faceNum; i++) {
				glFrontFace(GL_CCW);//设置逆时针方向为正向，可将内环面剔除
				int j = brep.faceArray[i].outerLoopIndex;
				//如果此面有内环，则开启模板测试，没有则不开启，防止后面的面无法绘制
				if (brep.faceArray[i].innerLoopNum != 0)
					glEnable(GL_STENCIL_TEST);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(faceColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				glDisable(GL_STENCIL_TEST);
			}

			//第二次进行线框的绘制
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//使用线框模式绘制
			glDisable(GL_STENCIL_TEST);
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);//设置模板测试方式
			for (int i = 0; i < brep.faceNum; i++) {
				glFrontFace(GL_CCW);
				int j = brep.faceArray[i].outerLoopIndex;
				//如果此面有内环，则开启模板测试，没有则不开启，防止后面的面无法绘制
				if (brep.faceArray[i].innerLoopNum != 0)
					glEnable(GL_STENCIL_TEST);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				myShader.setVec4("lineColor", glm::vec4(outlineColor, 1.0f));
				glDrawElements(GL_TRIANGLE_FAN, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				//绘制内环线框
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
			//实体纯线框显示模式
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
