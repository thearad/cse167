#include "HeightMap.h"

HeightMap::HeightMap() {
}

void HeightMap::init() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO_vert);
	glGenBuffers(1, &VBO_norm);
	glGenBuffers(1, &VBO_tex);

	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	//Vertices
	glBindBuffer(GL_ARRAY_BUFFER, VBO_vert);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), &vertices[0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indices.size(), &indices[0], GL_STATIC_DRAW);

	// Vertex Normals
	glBindBuffer(GL_ARRAY_BUFFER, VBO_norm);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), &normals[0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);

	// Texture Coordinates
	glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*texCoords.size(), &texCoords[0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (GLvoid*)0);

	//Reset buffer and vertex binds to 0
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void HeightMap::loadTextures() {
	/*TODO: make the textures pass-in-able. Set max # of textures to smth*/
	string path = "../textures/";
	std::vector<string> sTextureNames = {
		"sand.jpg", "grass_4.jpg", "grass_3.jpg", "grass_rock.jpg", "snow.jpg"
	};
	for (int i = 0; i < sTextureNames.size(); i++) {
		textures.push_back(Texture(path + sTextureNames[i], "texSampler" + std::to_string(i)));
	}
}

void HeightMap::draw(GLuint shaderProgram) {
	//Send misc. data
	glm::mat4 model = glm::mat4(1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &Window::P[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &Window::V[0][0]);

	//Bind all available textures. Send texture bind location to corresponding uniform shader_var
	//See:https://www.opengl.org/wiki/Sampler_(GLSL) under Binding textures to samplers
	for (int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i); //switches texture bind location to GL_TEXTURE(0+i)
		glBindTexture(GL_TEXTURE_2D, textures[i].getID()); //bind texture to active location
		glUniform1i(glGetUniformLocation(shaderProgram, textures[i].getShaderVar().c_str()), i); //sets uniform sampler2D texSampleri's texture bind loc.

	}

	// Draw HeightMap
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//unbind textures
	for (int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i); //switches texture bind location to GL_TEXTURE(0+i)
		glBindTexture(GL_TEXTURE_2D, 0); //bind texture to active location
	}
}

void HeightMap::drawNormals(GLuint shaderProgram) {
	glm::mat4 mvp = Window::P* Window::V * glm::mat4(1.f);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, &mvp[0][0]);
	glBindVertexArray(VAO);
	glDrawElements(GL_POINTS, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

}

/*
Generates a x by z heightmap vertices and texture coordinates
*/
void HeightMap::genVertices(int x, int z) {
	srand((unsigned int)time(NULL));
	PerlinNoise pn(0.05, 0.2f, 35.f, 5, rand() % 100);
	width = x;
	height = z;

	float y_mid = height / 2;
	float x_mid = width / 2;
	float max_distance = sqrt(pow(x_mid, 2) + pow(y_mid, 2));
	float max_width = sqrt(pow(x_mid, 2));

	std::vector<double> heights;

	double max_height = DBL_MIN;
	double min_height = DBL_MAX;

	int index_w = 0, index_h=0;
	for (int h = -height / 2; h < height / 2; h++) {
		for (int w = -width / 2; w < width / 2; w++) {
			double height = pn.GetHeight(index_w, index_h);

			float dist_x = pow(0.f - (float)w, 2);
			float dist_y = pow(0.f - (float)h, 2);
			float dist = sqrt(dist_x + dist_y);
			float dist_ratio = dist / max_width;

			float gradient = dist_ratio * dist_ratio;
			gradient = fmax(0.f, 1.f - gradient);

			vertices.push_back(glm::vec3(w, gradient*abs(height), h));
			index_w++;
		}
		index_h++;
		index_w = 0;
	}

	calcNormals();
	calcIndices();
	calcTexCoords();
}

void HeightMap::calcNormals() {
	//http://www.mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=24
	vector<std::vector<glm::vec3>> vNormals[2];
	for (int i = 0; i < 2; i++) {
		vNormals[i] = vector< vector<glm::vec3> >(height - 1, vector<glm::vec3>(width - 1));
	}
	for (int h = 0; h < height - 1; h++) {
		for (int w = 0; w < width - 1; w++) {
			glm::vec3 vTriangle0[] =
			{
				vertices[h*width + w],
				vertices[(h + 1)*width + w],
				vertices[(h + 1)*width + (w + 1)]
			};
			glm::vec3 vTriangle1[] =
			{
				vertices[(h + 1)*width + (w + 1)],
				vertices[h*width + (w + 1)],
				vertices[h*width + w]
			};

			vNormals[0][h][w] = getSurfaceNorm(vTriangle0[0], vTriangle0[1], vTriangle0[2]);
			vNormals[1][h][w] = getSurfaceNorm(vTriangle1[0], vTriangle1[1], vTriangle1[2]);
		}
	}
	for (int h = 0; h < height - 1; h++) {
		for (int w = 0; w < width - 1; w++) {
			glm::vec3 f_norm;

			if (h != 0 && w != 0)
				for (int k = 0; k<2; k++)
					f_norm += vNormals[k][h - 1][w - 1];

			// Look for upper-right triangles
			if (h != 0 && w != width - 1)
				f_norm += vNormals[0][h - 1][w];

			// Look for bottom-right triangles
			if (h != height - 1 && w != height - 1)
				for (int k = 0; k < 2; k++)
					f_norm += vNormals[k][h][w];

			// Look for bottom-left triangles
			if (h != height - 1 && w != 0)
				f_norm += vNormals[1][h][w - 1];

			normals.push_back(glm::normalize(f_norm));
		}
	}
}

void HeightMap::calcIndices() {
	for (int h = 0; h < height - 1; h++) {
		for (int w = 0; w < width - 1; w++) {
			indices.push_back(h*width + w);
			indices.push_back((h + 1)*width + w);
			indices.push_back((h + 1)*width + (w + 1));

			indices.push_back((h + 1)*width + (w + 1));
			indices.push_back(h*width + (w + 1));
			indices.push_back(h*width + w);
		}
	}
}

void HeightMap::calcTexCoords() {
	float fTextureU = float(width)*0.25f;
	float fTextureV = float(height)*0.25f;

	for (int h = 0; h < height; h++) {
		for (int w = 0; w < width; w++) {
			float fScaleC = float(w) / float(width);
			float fScaleR = float(h) / float(height);

			//fScales range from [0-1]. fTexture = fraction of width & height
			texCoords.push_back(glm::vec2(fTextureU*fScaleC, fTextureV*fScaleR));
		}
	}
}