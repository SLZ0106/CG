#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <CanvasPoint.h>
#include <ModelTriangle.h>
#include <TextureMap.h>
#include <Colour.h>
#include <Utils.h>
#include <RayTriangleIntersection.h>
#include <fstream>
#include <vector>
#include <map>
#include <glm/glm.hpp>


#define WIDTH 320
#define HEIGHT 240
glm::vec3 cameraPosition (0.0, 0.0, 4.0);
glm::mat3 cameraOrientation (1, 0, 0, 0, 1, 0 ,0, 0, 1);
glm::mat3 Rotation(1, 0, 0, 0, 1, 0 ,0, 0, 1);
float x = 0.0;
float y = 0.0;
glm::vec3 newCameraPosition = cameraPosition;
float depthBuffer[HEIGHT][WIDTH];

void clearDepthBuffer(){
    for(int y = 0; y < HEIGHT; y++)
        for(int x = 0; x < WIDTH; x++)
            depthBuffer[y][x] = INT32_MIN;
}

// ---------------------- Week 1 ---------------------- //
void draw(DrawingWindow &window) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = rand() % 256;
			float green = 0.0;
			float blue = 0.0;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

// ---------------------- Week 2 ---------------------- //
std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValue) {
    std::vector<float> result;
    float difference = (to - from) / float(numberOfValue - 1);
    for(int i=0; i<numberOfValue; i++) result.push_back(from + (float)i*difference);
    return result;
}

void greyscaleInterpolation(DrawingWindow &window) {
    window.clearPixels();
    std::vector<float> interpolation = interpolateSingleFloats(255, 0, int(window.width));
    for (size_t x = 0; x < window.width; x++) {
        float red = interpolation[x];
        float green = interpolation[x];
        float blue = interpolation[x];
        for (size_t y = 0; y < window.height; y++) {
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValue) {
    std::vector<glm::vec3> result;
    glm::vec3 difference = (to - from) * (1 / (float(numberOfValue) - 1));
    for(float i=0.0; i<numberOfValue; i++) {
        result.push_back(from + difference*float(i));
    }
    return result;
}

void twoDimensionalColourInterpolation(DrawingWindow &window) {
    window.clearPixels();
    glm::vec3 topLeft(255, 0, 0);        // red
    glm::vec3 topRight(0, 0, 255);       // blue
    glm::vec3 bottomRight(0, 255, 0);    // green
    glm::vec3 bottomLeft(255, 255, 0);   // yellow
    std::vector<glm::vec3> left = interpolateThreeElementValues(topLeft, bottomLeft, int(window.height));
    std::vector<glm::vec3> right = interpolateThreeElementValues(topRight, bottomRight, int(window.width));
    for(size_t y=0; y<window.height; y++) {
        std::vector<glm::vec3> row = interpolateThreeElementValues(left[y], right[y], int(window.width));
        for(size_t x=0; x<window.width; x++) {
            float red = row[x].x;
            float green = row[x].y;
            float blue = row[x].z;
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

// ---------------------- Week 3 ---------------------- //
void drawLine(DrawingWindow &window, const CanvasPoint from, const CanvasPoint to, Colour colour) {
    // Calculate step size
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float numberOfSteps = fmax(fabs(xDiff), fabs(yDiff));
    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float depthStepSize = (to.depth - from.depth)/numberOfSteps;
    // Calculate colour
    int red = colour.red;
    int green = colour.green;
    int blue = colour.blue;
    uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
    // Set colour for pixel
    if (x>=0 && y>=0 && x<WIDTH && y<HEIGHT) {
        for (float i = 0.0; i < numberOfSteps; i++) {
            float x = from.x + (xStepSize * i);
            float y = from.y + (yStepSize * i);
            float z = from.depth + (depthStepSize * i);
            if(depthBuffer[int(round(y))][int(round(x))] <= z) {
                depthBuffer[int(round(y))][int(round(x))] = z;
                window.setPixelColour(int(round(x)), int(round(y)), c);
            }
        }
    }
}

void drawStrokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    drawLine(window, triangle.v0(), triangle.v1(), colour);
    drawLine(window, triangle.v1(), triangle.v2(), colour);
    drawLine(window, triangle.v2(), triangle.v0(), colour);
}

// judge triangle is top-to-bottom or bottom-to-top
int sign(float d){
    if (d<0) return -1; // bottom-to-top
    else if (d == 0) return 0;
    else return 1; // top-to-bottom
}

void triangleRasteriser(DrawingWindow &window, CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour colour) {
    float dy = fabs(v2.y-v1.y);
    float x = v1.x;
    float y = v1.y;
    float x2 = v2.x;
    float x3 = v3.x;
    float y3 = v3.y;
    float s = sign(y - y3);
    float z1 = v2.depth - v1.depth;
    float z2 = v3.depth - v1.depth;
    CanvasPoint newV1=v1;
    CanvasPoint newV2=v1;
    for (float i = 1.00; i<=dy; i++)  {
        float f = i / dy;
        newV1.x = x + (x2 - x) * f; // Calculate the start point
        newV1.y -= s; // move y with 1 step each time
        newV1.depth = v1.depth + z1 * f;
        newV2.x = x + (x3 - x) * f; // Calculate the end point
        newV2.y -= s; // move y with 1 step each time
        newV2.depth = v1.depth + z2 * f;
        drawLine(window, newV1, newV2, colour);
    }

}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    //drawStrokedTriangle(window, triangle, Colour(255, 255, 255));
    // sort point
    CanvasPoint topPoint;
    CanvasPoint bottomPoint;
    CanvasPoint givenPoint;
    CanvasPoint extraPoint;
    if(triangle.v0().y <= triangle.v1().y && triangle.v0().y <= triangle.v2().y) {
        topPoint = triangle.v0();
        if(triangle.v1().y >= triangle.v2().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v1();
        }
    } else if(triangle.v1().y <= triangle.v0().y && triangle.v1().y <= triangle.v2().y) {
        topPoint = triangle.v1();
        if(triangle.v0().y >= triangle.v2().y) {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v0();
        }
    } else if(triangle.v2().y <= triangle.v0().y && triangle.v2().y <= triangle.v1().y) {
        topPoint = triangle.v2();
        if(triangle.v1().y >= triangle.v0().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v0();
        } else {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v1();
        }
    }
    // separate triangle
    double similarTriangleRadio = (bottomPoint.x - topPoint.x)/(bottomPoint.y - topPoint.y);
    float extraPointX;
    float extraPointY;
    float extraPointDepth;
    // when radio is negative, the extra point is behind on the topPoint (-); when it is positive, the extraPoint is in the front of topPoint (+)
    if(similarTriangleRadio >= 0) {
        extraPointX = topPoint.x + ((givenPoint.y - topPoint.y) * fabs(similarTriangleRadio));
    } else {
        extraPointX = topPoint.x - ((givenPoint.y - topPoint.y) * fabs(similarTriangleRadio));
    }
    extraPointY = givenPoint.y;
    float alpha = (-(extraPointX-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(extraPointY-triangle.v1().y)*(triangle.v2().x-triangle.v1().x))/(-(triangle.v0().x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(triangle.v0().y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x));
    float beta = (-(extraPointX-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(extraPointY-triangle.v2().y)*(triangle.v0().x-triangle.v2().x))/(-(triangle.v1().x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(triangle.v1().y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x));
    float gamma = 1 - alpha - beta;
    extraPointDepth = alpha * triangle.v0().depth + beta * triangle.v1().depth + gamma * triangle.v2().depth;
    extraPoint = CanvasPoint(extraPointX, givenPoint.y, extraPointDepth);

    // filled the first triangle
    triangleRasteriser(window, topPoint, givenPoint, extraPoint, colour);
    // filled the second triangle
    triangleRasteriser(window, bottomPoint, givenPoint, extraPoint, colour);
}

// Read texture map from image file
TextureMap getTextureMap(const std::string& image) {
    TextureMap textureMap = TextureMap(image);
    std::cout << "width " << textureMap.width << "height " << textureMap.height << std::endl;
    return textureMap;
}

// Match the point in texture map
uint32_t textureMapper(CanvasTriangle triangle, CanvasPoint point, TextureMap textureMap){
    // Calculate barycentric coordinates
    float alpha = (-(point.x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(point.y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x))/(-(triangle.v0().x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(triangle.v0().y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x));
    float beta = (-(point.x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(point.y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x))/(-(triangle.v1().x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(triangle.v1().y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x));
    float gamma = 1 - alpha - beta;
    // Get texture point
    CanvasPoint texturePoint((alpha * triangle.v0().texturePoint.x + beta * triangle.v1().texturePoint.x + gamma * triangle.v2().texturePoint.x), (alpha * triangle.v0().texturePoint.y + beta * triangle.v1().texturePoint.y + gamma * triangle.v2().texturePoint.y));
    // Locate the texture point position in texture map pixel list
    int index = int(texturePoint.y) * textureMap.width + int(texturePoint.x);
    std::cout << "Index of texture is:" << index << " corresponds to texture point x:" << texturePoint.x << " and y: " << texturePoint.y << "\n" << std::endl;
    uint32_t colour = textureMap.pixels[index - 1];
    return colour;
}

void textureTriangle(DrawingWindow &window, const TextureMap& textureMap, CanvasTriangle triangle, CanvasPoint v1, CanvasPoint v2, CanvasPoint v3) {
    float dy = fabs(v2.y-v1.y);
    int x = v1.x;
    int y = v1.y;
    int x2 = v2.x;
    int x3 = v3.x;
    int y3 = v3.y;
    int s = sign(y - y3);
    CanvasPoint newV1=v1;
    CanvasPoint newV2=v1;
    for (int i = 1; i<=dy; i++)  {
        float f = i / dy;
        newV1.x = x + (x2 - x) * f; // Calculate the start point
        newV1.y -= s; // move y with 1 step each time
        newV2.x = x + (x3 - x) * f; // Calculate the end point
        newV2.y -= s; // move y with 1 step each time
        // Calculate step size
        float numberOfSteps = newV2.x - newV1.x;
        // Set colour for pixel on one line
        for (float j = 0.0; j < numberOfSteps; j++) {
            float x = newV1.x + j;
            float y = newV1.y;
            uint32_t colour = textureMapper(triangle, CanvasPoint(x, y), textureMap);
            window.setPixelColour(x, y, colour);
        }
    }
}

void drawTextureTriangle(DrawingWindow &window, const TextureMap& textureMap, CanvasTriangle triangle) {
    // Draw the triangle
    drawStrokedTriangle(window, triangle, Colour(255,255,255));
    // sort point
    CanvasPoint topPoint;
    CanvasPoint bottomPoint;
    CanvasPoint givenPoint;
    CanvasPoint extraPoint;
    if(triangle.v0().y <= triangle.v1().y && triangle.v0().y <= triangle.v2().y) {
        topPoint = triangle.v0();
        if(triangle.v1().y >= triangle.v2().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v1();
        }
    } else if(triangle.v1().y <= triangle.v0().y && triangle.v1().y <= triangle.v2().y) {
        topPoint = triangle.v1();
        if(triangle.v0().y >= triangle.v2().y) {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v0();
        }
    } else if(triangle.v2().y <= triangle.v0().y && triangle.v2().y <= triangle.v1().y) {
        topPoint = triangle.v2();
        if(triangle.v1().y >= triangle.v0().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v0();
        } else {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v1();
        }
    }
    // separate triangle
    double similarTriangleRadio = (bottomPoint.x - topPoint.x)/(bottomPoint.y - topPoint.y);
    float extraPointX;
    // when radio is negative, the extra point is behind on the topPoint (-); when it is positive, the extraPoint is in the front of topPoint (+)
    if(similarTriangleRadio >= 0) {
        extraPointX = topPoint.x + ((givenPoint.y - topPoint.y) * fabs(similarTriangleRadio));
    } else {
        extraPointX = topPoint.x - ((givenPoint.y - topPoint.y) * fabs(similarTriangleRadio));
    }
    extraPoint = CanvasPoint(extraPointX, givenPoint.y);

    // filled the first triangle
    textureTriangle(window, textureMap, triangle, topPoint, givenPoint, extraPoint);
    // filled the second triangle
    textureTriangle(window, textureMap, triangle, bottomPoint, givenPoint, extraPoint);
}

// ---------------------- Week 4 ---------------------- //
Colour mtlConverter(double r, double g, double b) {
    Colour packedColour = Colour(int(r*255), int(g*255), int(b*255));
    return packedColour;
}

std::map<std::string, Colour> mtlReader(const std::string& file) {
    std::map<std::string, Colour> colours;
    std::string myText;
    std::string name;
    std::ifstream File(file);
    while(getline(File, myText)) {
        std::vector<std::string> text = split(myText, ' ');
        if(text[0] == "newmtl") {
            name = text[1];
        } else if (text[0] == "Kd") {
            colours[name] = mtlConverter(std::stod(text[1]), std::stod(text[2]), std::stod(text[3]));
        }
    }
    File.close();
    return colours;
}

std::vector<ModelTriangle> objReader(const std::string& objFile, const std::string& mtlFile, float scalingFactor) {
    std::vector<glm::vec3> vertex;
    std::vector<std::vector<std::string>> facets;
    std::vector<ModelTriangle> modelTriangles;
    std::string myText;
    std::string colourName;
    std::ifstream File(objFile);
    while(getline(File, myText)) {
        std::vector<std::string> text = split(myText, ' ');
        if(text[0] == "usemtl") {
            colourName = text[1];
        } else if(text[0] == "v") {
            glm::vec3 v = glm::vec3(std::stod(text[1]), std::stod(text[2]), std::stod(text[3]));
            vertex.push_back(v);
        } else if(text[0] == "f") {
            std::vector<std::string> f {text[1], text[2], text[3], colourName};
            facets.push_back(f);
        }
    } 
    File.close();
    std::map<std::string, Colour> colourMap = mtlReader(mtlFile);
    for(std::vector<std::string> i : facets) {
        glm::vec3 v0 = vertex[std::stoi(i[0])-1];
        glm::vec3 v1 = vertex[std::stoi(i[1])-1];
        glm::vec3 v2 = vertex[std::stoi(i[2])-1];
        glm::vec3 normal = glm::normalize(glm::cross(v1-v0, v2-v0));
        Colour colour = colourMap[i[3]];
        ModelTriangle triangle = ModelTriangle(v0*scalingFactor, v1*scalingFactor, v2*scalingFactor, colour);
        triangle.normal = normal;
        modelTriangles.push_back(triangle);
    }

    return modelTriangles;
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::mat3 cameraOrientation, glm::vec3 vertexPosition, float focallength, float scalingFactor) {
    CanvasPoint twoDPoint;
    glm::vec3 cameraToVertex = {vertexPosition.x - cameraPosition.x, vertexPosition.y - cameraPosition.y, vertexPosition.z - cameraPosition.z};
    glm::vec3 cameraToVertexInCameraSpace =  cameraToVertex * cameraOrientation;
    twoDPoint.x = round(focallength * (-scalingFactor) * cameraToVertexInCameraSpace.x / cameraToVertexInCameraSpace.z + float(WIDTH/2));
    twoDPoint.y = round(focallength * scalingFactor * cameraToVertexInCameraSpace.y / cameraToVertexInCameraSpace.z + float(HEIGHT/2));
    twoDPoint.depth = 1/fabs(cameraToVertexInCameraSpace.z);
    return twoDPoint;
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec3 cameraPosition, glm::mat3 cameraOrientation, ModelTriangle triangle, float focallength, float scalingFactor) {
    CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[0], focallength, scalingFactor);
    CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[1], focallength, scalingFactor);
    CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[2], focallength, scalingFactor);
    CanvasTriangle canvasTriangle = CanvasTriangle(v0, v1, v2);
    return canvasTriangle;
}

void PointCloud(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour) {
    int red = colour.red;
    int green = colour.green;
    int blue = colour.blue;
    uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        window.setPixelColour(canvasTriangle.v0().x, canvasTriangle.v0().y, c);
        window.setPixelColour(canvasTriangle.v1().x, canvasTriangle.v1().y, c);
        window.setPixelColour(canvasTriangle.v2().x, canvasTriangle.v2().y, c);
    }
}

void WireFrame(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour){
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        drawLine(window, canvasTriangle.v0(), canvasTriangle.v1(), triangle.colour);
        drawLine(window, canvasTriangle.v1(), canvasTriangle.v2(), triangle.colour);
        drawLine(window, canvasTriangle.v2(), canvasTriangle.v0(), triangle.colour);
    }
}

void Rasterised(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour) {
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        //ImageDepth(canvasTriangle, Colour(255, 255, 255));
        drawFilledTriangle(window, canvasTriangle, triangle.colour);
    }
}

glm::mat3 lookAt(glm::vec3 cameraPosition) {
    glm::vec3 vertical (0, 1, 0);
    glm::vec3 center (0, 0, 0);
    glm::vec3 forward = glm::normalize(cameraPosition - center);
    glm::vec3 right = glm::cross(vertical, forward);
    glm::vec3 up = glm::cross(forward, right);
    glm::mat3 newCameraOrientation = glm::mat3(right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
    return newCameraOrientation;
}

RayTriangleIntersection getClosestIntersection(const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::vec3 rayDirection){
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = FLT_MAX;
    for(size_t i = 0; i < triangles.size(); i++) {
        glm::vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
        glm::vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
        glm::vec3 SPVector = cameraPosition - triangles[i].vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
        if(possibleSolution[0] <= closestIntersection.distanceFromCamera && possibleSolution[0] >= 0 && possibleSolution[1] >= 0 && possibleSolution[1] <= 1 && possibleSolution[2] >= 0 && possibleSolution[2] <= 1 && (possibleSolution[1] + possibleSolution[2]) <= 1){
            glm::vec3 intersectionPoint = cameraPosition + possibleSolution[0] * rayDirection;
            closestIntersection = RayTriangleIntersection(intersectionPoint, possibleSolution[0], triangles[i], i);
        }
    }
    return closestIntersection;
//    if(intersectionVector.size() == 0) {
//        return closestIntersection;
//    }else{
//        float minDistance = intersectionVector[0].distanceFromCamera;
//        int minIndex = 0;
//       for(size_t i = 1; i < intersectionVector.size(); i++) {
//            if(intersectionVector[i].distanceFromCamera < minDistance) {
//                minDistance = intersectionVector[i].distanceFromCamera;
//                minIndex = i;
//            }
//        }
//        return intersectionVector[minIndex];
//    }
}

glm::vec3 get3DPoint(CanvasPoint point, glm::vec3 cameraPosition, float focalLength, float scalingFactor) {
    float u = point.x;
    float v = point.y;
    float z = cameraPosition.z - focalLength;
    float x = ((u - (float(WIDTH)/2)) / (focalLength * (scalingFactor))) * z;
    float y = ((v - (float(HEIGHT)/2)) / (focalLength * (-scalingFactor))) * z;
    glm::vec3 threeDPoint (x, y, z);
    return threeDPoint;
}

void drawRasterisedScene(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            red = colour.red;
            green = colour.green;
            blue = colour.blue;
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
}

void drawRasterisedShadowScene(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            int S = 8;
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = closestIntersection.intersectionPoint - lightPosition;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = 0;
                green = 0;
                blue = 0;
            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*intensity, 255);
                green = fmin(colour.green*intensity, 255);
                blue = fmin(colour.blue*intensity, 255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 

void drawIncidence(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            int S = 8;
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = glm::normalize(closestIntersection.intersectionPoint - lightPosition);
            ModelTriangle triangle = closestIntersection.intersectedTriangle;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            float incidenceintensity = glm::clamp<float>(glm::dot(triangle.normal, -lightDirection), 0.0, 1.0);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = 0;
                green = 0;
                blue = 0;
            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*incidenceintensity*intensity,255);
                green = fmin(colour.green*incidenceintensity*intensity,255);
                blue = fmin(colour.blue*incidenceintensity*intensity,255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 

void drawSpecular(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            int S = 8;
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = glm::normalize(closestIntersection.intersectionPoint - lightPosition);
            ModelTriangle triangle = closestIntersection.intersectedTriangle;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            glm::vec3 reflection = glm::normalize(lightDirection - (2*glm::dot(lightDirection, triangle.normal)*triangle.normal));
            glm::vec3 view = glm::normalize(cameraPosition - closestIntersection.intersectionPoint);
            float specularintensity = glm::dot(view, reflection);
            specularintensity = pow(specularintensity, 256);
            float incidenceintensity = glm::clamp<float>(glm::dot(triangle.normal, -lightDirection), 0.0, 1.0);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = 0;
                green = 0;
                blue = 0;
            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*(incidenceintensity*intensity + specularintensity)+25,255);
                green = fmin(colour.green*(incidenceintensity*intensity + specularintensity)+25,255);
                blue = fmin(colour.blue*(incidenceintensity*intensity + specularintensity)+25,255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 


// ---------------------- Show in window ---------------------- //
void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) {
            std::cout << "LEFT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            std::cout << "RIGHT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_UP) {
            std::cout << "UP" << std::endl;
        } else if (event.key.keysym.sym == SDLK_DOWN) {
            std::cout << "DOWN" << std::endl;
        } else if (event.key.keysym.sym == SDLK_u) {
            CanvasPoint point1,point2,point3;
            CanvasTriangle triangle;
            point1 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point2 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point3 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            triangle = CanvasTriangle(point1, point2, point3);
            drawStrokedTriangle(window, triangle, Colour(rand()%256, rand()%256, rand()%256));
        } else if (event.key.keysym.sym == SDLK_f) {
            CanvasPoint point1,point2,point3;
            CanvasTriangle triangle;
            point1 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point2 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point3 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            triangle = CanvasTriangle(point1, point2, point3);
            drawStrokedTriangle(window, triangle, Colour(255, 255, 255));
            drawFilledTriangle(window, triangle, Colour(rand()%256, rand()%256, rand()%256));
        } else if(event.key.keysym.sym == SDLK_o) {
            CanvasPoint point1,point2,point3;
            point1 = CanvasPoint(160, 10);
            point2 = CanvasPoint(300, 230);
            point3 = CanvasPoint(10, 150);
            point1.texturePoint.x = 195; point1.texturePoint.y = 5;
            point2.texturePoint.x = 395; point2.texturePoint.y = 380;
            point3.texturePoint.x = 65; point3.texturePoint.y = 330;
            drawTextureTriangle(window, getTextureMap("texture.ppm"), CanvasTriangle(point1, point2, point3));
        } else if(event.key.keysym.sym == SDLK_p) {
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            PointCloud(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            WireFrame(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if(event.key.keysym.sym == SDLK_w) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.y -= 0.05;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_s) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.y += 0.05;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_a) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.x += 0.05;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_d) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.x -= 0.05;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            PointCloud(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            WireFrame(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_q) {
            //left
            window.clearPixels();
            clearDepthBuffer();
            x = 1.0;
            float radianx = glm::radians(x);
            glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, sin(radianx), 0, 1, 0, -sin(radianx), 0, cos(radianx));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_e) {
            //right
            window.clearPixels();
            clearDepthBuffer();
            x = 1.0;
            float radianx = glm::radians(x);
            glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, -sin(radianx), 0, 1, 0, sin(radianx), 0, cos(radianx));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;   
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));  
        } else if (event.key.keysym.sym == SDLK_z) {
            //downwards
            window.clearPixels();
            clearDepthBuffer();
            y = 0.3;
            float radiany = glm::radians(y);
            glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radiany), sin(radiany), 0, -sin(radiany), cos(radiany));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_x) {
            //upwards
            window.clearPixels();
            clearDepthBuffer();
            y = 0.3;
            float radiany = glm::radians(y);
            glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radiany), -sin(radiany), 0, sin(radiany), cos(radiany));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_r) {
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            drawRasterisedScene(window, modelTriangles, cameraPosition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_t) {
            glm::vec3 lightposition = glm::vec3(0.0, 0.5, 0.0);
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_y) {
            glm::vec3 lightposition = glm::vec3(0.0, 0.4, 0.3);
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35); 
            drawIncidence(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_m) {
            glm::vec3 lightposition = glm::vec3(0.0, 0.4, 0.3);
            float focalLength = 2.0;
            auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35); 
            drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_c) {
            clearDepthBuffer();
            window.clearPixels();
            }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}
int main(int argc, char *argv[]) {
     // Single Element Numerical Interpolation
//     std::vector<float> result;
//     result = interpolateSingleFloats(2.2, 8.5, 7);
//     for(float i : result) std::cout << i << " ";
//     std::cout << std::endl;

    // Three Element Numerical Interpolation
//     std::vector<glm::vec3> result;
//     glm::vec3 from(1, 4, 9.2);
//     glm::vec3 to(4, 1, 9.8);
//     result = interpolateThreeElementValues(from, to, 4);
//     for(size_t i=0; i<result.size(); i++) std::cout << result[i].x << " " << result[i].y << " " << result[i].z << " " << std::endl;
//    CanvasPoint point1,point2,point3;
//    point1 = CanvasPoint(160, 10);
//    point2 = CanvasPoint(300, 230);
//    point3 = CanvasPoint(10, 150);
//    point1.texturePoint.x = 195; point1.texturePoint.y = 5;
//    point2.texturePoint.x = 395; point2.texturePoint.y = 380;
//    point3.texturePoint.x = 65; point3.texturePoint.y = 330;
//   textureMapper(CanvasTriangle(point1, point2, point3), point1, getTextureMap("./texture.ppm"));

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
        // week 1
//        draw(window);
        // week 2
//        greyscaleInterpolation(window);
//        twoDimensionalColourInterpolation(window);
        // week 3
//        drawLine(window, CanvasPoint(window.width/2, window.height/2), CanvasPoint(window.width-1, window.height-1), Colour(255, 255, 255));
//        drawLine(window, CanvasPoint(window.width/2, 0), CanvasPoint(window.width/2, window.height/2), Colour(255, 0, 0));
//        drawLine(window, CanvasPoint(0, window.height-1), CanvasPoint(window.width-1, 0), Colour(0, 0, 255));
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
        //Orbit
            //auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            //float radianx = 0.01;
            //window.clearPixels();
            //clearDepthBuffer();
            // y axis anticlockwise
            //glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, sin(radianx), 0, 1, 0, -sin(radianx), 0, cos(radianx));
            // y axis clockwise
            //glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, -sin(radianx), 0, 1, 0, sin(radianx), 0, cos(radianx));
            // x axis anticlockwise
            //glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radianx), -sin(radianx), 0, sin(radianx), cos(radianx));
            // use only in y axis rotation
            //Rotation = lookAt(cameraPosition);
            // use only in y axis rotation
            //Rotation = Rotation * cameraRotation;
            //cameraPosition =  cameraRotation * cameraPosition;
            //float focalLength = 2.0;
            //Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));

		window.renderFrame();
	}
}
