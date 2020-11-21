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

//摄像机
Camera camera(glm::vec3(0.0f, 0.0f, 20.0f));
float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间
bool firstMouse = true;

//光源
glm::vec3 lightPos = glm::vec3(-20.0f, 40.0f, 20.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

//实体颜色
glm::vec3 outlineColor = glm::vec3(1.0f, 0.5f, 0.2f);
glm::vec3 faceColor = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 ringColor = glm::vec3(0.2f, 0.5f, 1.0f);

//控制变量
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
	std::cout << "1：创建模式，将从OperationInput.txt文件中读取数据创建实体模型\n2：读取模式，将读取breps.brp文件数据进行显示\n请输入运行模式编号:";
	std::cin >> selectMode;
	while (selectMode != 1 && selectMode != 2) {
		std::cout << "无效输入！" << std::endl;
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

//读取操作数据创建实体，并将数据存储为.brp文件，默认名为Output.brp
void ModelDataCreate(const char filename[]) {
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
	int innerLoopVertexNum, inputInnerLoopNum;
	Vertex* firstInnerVertex;
	Face* innerLoopFace, * innerFace;
	Loop* remainLoop;

	//输入的内环-扫成操作数，每个内环创建操作跟一个扫成操作
	brepFile >> inputInnerLoopNum;
	for (int k = 0; k < inputInnerLoopNum; k++) {
		//第二个内环
		brepFile >> innerLoopVertexNum >> position.x >> position.y >> position.z;
		//寻找内环所在面
		innerLoopFace = FindFaceAtPoint(position, solid->faceList);
		//在所在面上进行mev操作，保证内环方向
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
		//如果扫成后面与对面重合，则形成通孔
		innerLoopFace = FindFaceAtPoint(innerFace->loop->he->startv->pos, solid->faceList, innerFace);
		if (innerLoopFace != NULL) {
			remainLoop = kfmrh(innerFace);
			innerLoopFace->innerLoopList.emplace_back(remainLoop);
		}
	}
		
	/*
	*/

	//将拓扑数据转为给GPU使用的数组数据
	brep.ListToArray();
	//打印数据
	brep.PrintInfo();
	brep.OutputData(filename);
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

//摄像头移动
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
void ModelDataRead(const char filename[]) {
	std::ifstream brepFile(filename);
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
		//取三点计算面的法向
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

//定义显示模块
void Show() {
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);

	//glClearColor(0.2f, 0.4f, 0.3f, 1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glLineWidth(3);

	//存放网格化后三角形顶点坐标的数组
	std::vector<std::vector<float> > dataArray;
	for (int i = 0; i < brep.faceNum; i++) {
		int j = brep.faceArray[i].outerLoopIndex;
		//多边形网格化相关变量
		std::vector<p2t::Triangle*> triangles;
		std::vector<p2t::Point*> polyline;
		//判断面的朝向对数据进行变换降维
		//获取环上首点进行坐标变换的初始化
		int p = brep.loopArray[j][0];
		glm::vec3 pos1 = glm::vec3(brep.vertexArray[3 * p], brep.vertexArray[3 * p + 1], brep.vertexArray[3 * p + 2]);
		//将平面点变换到局部坐标系中，坐标系顶点为首点坐标，z轴为法向，y轴由计算得出(若存在法向为(1, 1, 1)的面则可能报错，此程序为方便只进行简单考虑)
		glm::vec3 front = brep.faceArray[i].normalDir;
		glm::mat4 trans = glm::lookAt(pos1, pos1 + front, glm::vec3(1.0f, 1.0f, 1.0f) - glm::vec3(abs(front.x), abs(front.y), abs(front.z)));
		//对外环每点进行坐标变换到局部坐标系下，并降维添加至polyline数组中
		for (int k = 0; k < brep.loopArray[j].size(); k++) {
			p = brep.loopArray[j][k];
			glm::vec3 pos = glm::vec3(brep.vertexArray[3 * p], brep.vertexArray[3 * p + 1], brep.vertexArray[3 * p + 2]);
			//坐标系变换
			glm::vec4 ans = trans * glm::vec4(pos, 1.0f);
			polyline.push_back(new p2t::Point(ans.x, ans.y));
		}
		p2t::CDT* cdt = new p2t::CDT(polyline);
		//对内环进行操作
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
				//添加孔洞
				cdt->AddHole(holeline);
			}
		}
		//进行多边形网格化
		cdt->Triangulate();
		triangles = cdt->GetTriangles();

		//暂存输出点
		std::vector<float> tempArray;
		for (int k = 0; k < triangles.size(); k++) {
			p2t::Triangle& t = *triangles[k];
			for (int j = 0; j < 3; j++) {
				p2t::Point& a = *t.GetPoint(j);
				glm::vec4 ans = glm::vec4(a.x, a.y, 0.0f, 1.0f);
				//对网格化后的点反变换到原位置
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
	//事先绑定顶点数组VAO，这样所有VBO和EBO的操作都会被存储至VAO中
	glBindVertexArray(VAO);
	//设置VBO与其相应的顶点数据
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_STATIC_DRAW);
	//设置VBO与其相应的顶点数据
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//设置GPU顶点解析格式以及使用的格式
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

		//观察矩阵
		glm::mat4 model = glm::mat4(1.0f);
		//投影矩阵
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(camera.Zoom), ScreenWidth / ScreenHeight, 0.1f, 100.0f);

		solidShader.use();
		solidShader.setMat4("model", model);
		solidShader.setMat4("view", camera.GetViewMatrix());
		solidShader.setMat4("projection", projection);
		solidShader.setVec3("viewPos", camera.Position);
		solidShader.setBool("isFrameMode", true);

		glBindVertexArray(VAO);

		//依次画出各个面
		for (int i = 0, j; i < brep.faceNum; i++) {
			switch (showMode) {
			case 1:
				//纯面显示
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				solidShader.setVec3("color", faceColor);
				solidShader.setBool("isFrameMode", false);
				glBufferData(GL_ARRAY_BUFFER, dataArray[i].size() * sizeof(float), &dataArray[i][0], GL_DYNAMIC_DRAW);
				//设置GPU顶点解析格式以及使用的格式
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_TRIANGLES, 0, dataArray[i].size() / 6);
				break;
			case 2:
				//纯网格显示
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				solidShader.setVec3("color", outlineColor);
				glBufferData(GL_ARRAY_BUFFER, dataArray[i].size() * sizeof(float), &dataArray[i][0], GL_DYNAMIC_DRAW);
				//设置GPU顶点解析格式以及使用的格式
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_TRIANGLES, 0, dataArray[i].size() / 6);
				break;
			case 3:
				//纯线框显示模式
				//载入数据
				glBufferData(GL_ARRAY_BUFFER, brep.vertexArray.size() * sizeof(float), &brep.vertexArray[0], GL_DYNAMIC_DRAW);
				//设置GPU顶点解析格式以及使用的格式
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (void*)0);
				glEnableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
				//画外环
				j = brep.faceArray[i].outerLoopIndex;
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, brep.loopArray[j].size() * sizeof(int), &brep.loopArray[j][0], GL_DYNAMIC_DRAW);
				solidShader.setVec3("color", outlineColor);
				glDrawElements(GL_LINE_LOOP, brep.loopArray[j].size(), GL_UNSIGNED_INT, 0);
				//画内环
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
